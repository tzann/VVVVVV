#ifndef SOLVER_TERRAIN_H
#define SOLVER_TERRAIN_H

#include <cstddef>
#include <vector>
#include <set>
#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <functional>
#include <queue>

#include <SDL.h>

#include "Graphics.h"
#include "Map.h"
#include "Entity.h"
#include "UtilityClass.h"
#include "Game.h"
#include "Exit.h"
#include "Screen.h"

#include "solver/Constants.h"
#include "solver/Exceptions.h"
#include "solver/Geometry.h"
#include "solver/Heuristic.h"
#include "solver/Numerics.h"
#include "solver/Solver.h"
#include "solver/SolverClean.h"

using namespace Geometry;

namespace Terrain {
	static int colors[12][3] = {
		{ 0xff, 0x00, 0x00 }, { 0xff, 0x7f, 0x00 }, { 0xff, 0xff, 0x00 }, { 0x7f, 0xff, 0x00 },
		{ 0x00, 0xff, 0x00 }, { 0x00, 0xff, 0x7f }, { 0x00, 0xff, 0xff }, { 0x00, 0x7f, 0xff },
		{ 0x00, 0x00, 0xff }, { 0x7f, 0x00, 0xff }, { 0xff, 0x00, 0xff }, { 0xff, 0x00, 0x7f },
	};

	// --------------------------------------
	// Structs & Enums
	// --------------------------------------
	enum CollisionSetting {
		None,
		Walls,
		WallsAndSpikes,
	};

	struct RoomPosition {
		bool outside;
		int rx, ry;

		RoomPosition() {
			rx = -1;
			ry = -1;
			outside = false;
		}
		RoomPosition(int rx, int ry) : rx(rx), ry(ry) {
			outside = false;
		}
		RoomPosition(bool outside, int rx, int ry) : outside(outside), rx(rx), ry(ry) { }

		bool is_valid(void) const {
			return rx >= 0 && rx <= 20 && ry >= 0 && ry <= 20;
		}

		bool operator== (const RoomPosition& other) const {
			return (outside == other.outside && rx == other.rx && ry == other.ry);
		}
		bool operator!= (const RoomPosition& other) const {
			return !(*this == other);
		}
		bool operator< (const RoomPosition& other) const {
			if (outside != other.outside) {
				return outside < other.outside;
			} else if (ry != other.ry) {
				return ry < other.ry;
			} else if (rx != other.rx) {
				return rx < other.rx;
			}
			// Equality
			return false;
		}
		bool operator<=(const RoomPosition& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const RoomPosition& other) const {
			return !(*this < other);
		}
		bool operator> (const RoomPosition& other) const {
			return !(*this <= other);
		}

		IntVector GetNativeRoomCoords() {
			IntVector result;
			if (outside) {
				result.x = rx + 41;
				result.y = ry + 48;
			} else {
				result.x = rx + 100;
				result.y = ry + 100;
			}
			return result;
		}

		bool IsTower();
		
		static RoomPosition FromNativeRoomCoords(int rx, int ry) {
			RoomPosition result;
			if (100 <= rx && rx < 120 && 100 <= ry && ry < 120) {
				// Overworld
				result.rx = rx - 100;
				result.ry = ry - 100;
				result.outside = false;
			}
			else if (41 <= rx && rx < 61 && 48 <= ry && ry < 68) {
				// Outside Dimension VVVVVV
				// Actual range of rooms is rx [41, 54], ry [48, 56]
				result.rx = rx - 41;
				result.ry = ry - 48;
				result.outside = true;
			}
			else {
				VVV_exit(1);
			}
			return result;
		}
		static RoomPosition invalid(void) {
			return RoomPosition(-1, -1);
		}

		RoomPosition NextRoomUp() {
			return RoomPosition(outside, rx, (ry + 19) % 20);
		}
		RoomPosition NextRoomDown() {
			return RoomPosition(outside, rx, (ry + 1) % 20);
		}
		RoomPosition NextRoomLeft() {
			return RoomPosition(outside, (rx + 19) % 20, ry);
		}
		RoomPosition NextRoomRight() {
			return RoomPosition(outside, (rx + 1) % 20, ry);
		}
	};

	struct GlobalPosition {
		RoomPosition room;
		IntVector pos;

		GlobalPosition() { }
		GlobalPosition(RoomPosition room, IntVector pos) : room(room), pos(pos) { }

		// These functions are for SolverClean, not Terrain
		bool isAfter(Solver::CheckedCorner& c) {
			return c.isPosAfter(*this);
		}
		bool isBefore(Solver::CheckedCorner& c) {
			return c.isPosBefore(*this);
		}
		bool isInside(Solver::CheckedCorner& c) {
			return c.isPosInside(*this);
		}
	};

	struct LocalFrame {
		GlobalPosition origin;
		bool invX, invY;

		LocalFrame() {
			origin.room = RoomPosition(0, 0);
			origin.pos = IntVector(0, 0);
			invX = invY = false;
		}
		LocalFrame(GlobalPosition origin, bool invX, bool invY) : origin(origin), invX(invX), invY(invY) { }
	};

	struct Ray {
		IntVector origin;
		IntVector direction;

		Ray(int x, int y, int dx, int dy) {
			origin.x = x;
			origin.y = y;
			direction.x = dx;
			direction.y = dy;
		}
	};

	// The direction from the corner's perspective, i.e. in which direction is it pointing
	enum CornerType {
		TopLeft,
		TopRight,
		BottomRight,
		BottomLeft,
	};

	struct Corner {
		IntVector pos;
		CornerType type;
		int verticalGap, horizontalGap;
		int verticalNegativeGap, horizontalNegativeGap;
		int verticalWallLength, horizontalWallLength;
		bool simple, deadEnd;

		IntVector GetPrimaryDir(bool inverseGravity) const;
		IntVector GetSecondaryDir(bool inverseGravity) const;
		void Corner::GetSpeedRange(FloatInterval& vx, FloatInterval& vy, bool goingUp) const;
		Region GetConnectingRegion(bool inverseGravity) const;
		Region GetRegionBefore(bool inverseGravity) const;
		Region GetIntermediateRegion(void) const;
		Region GetRegionAfter(bool inverseGravity) const;
		Region Corner::GetValidRegion(bool goingUp) const;
	};

	enum WallType {
		Invalid,
		Floor,
		Ceiling,
		LeftWall,
		RightWall,
	};

	struct RoomWall {
		WallType type;
		int plane;			    // The coordinate of the wall on the perpendicular axis
		int min, max;			// The minimum and maximum coordinates of the wall on the parallel axis
		bool minCornerConcave;  // true if rays cannot pass through the min corner
		bool maxCornerConcave;  // true if rays cannot pass through the max corner

		bool walkable;

		Region GetConnectingRegion(void) const;
		Region GetBoundedRegion(void) const;
		Region GetBlockedRegion(void) const;
	};

	struct GravityLine {
		bool isHorizontal;
		IntVector min;
		IntVector max;

		GravityLine() {}
		GravityLine(bool isHorizontal, IntVector min, IntVector max) : isHorizontal(isHorizontal), min(min), max(max) { }

		Region GetConnectingRegion(void) const;
	};

	struct CornerID {
		RoomPosition room;
		int cornerIndex;
		bool goingUp;

		CornerID() : room(RoomPosition::invalid()), cornerIndex(-1), goingUp(false) {}
		CornerID(RoomPosition r, int i, bool goingUp) : room(r), cornerIndex(i), goingUp(goingUp) { }

		bool is_valid(void) const {
			return room.is_valid() && cornerIndex >= 0;
		}
		bool is_same_corner(const CornerID& other) const {
			return (room == other.room && cornerIndex == other.cornerIndex);
		}

		bool operator== (const CornerID& other) const {
			return (room == other.room && cornerIndex == other.cornerIndex && goingUp == other.goingUp);
		}
		bool operator!= (const CornerID& other) const {
			return !(*this == other);
		}

		bool operator< (const CornerID& other) const {
			if (room != other.room) {
				return room < other.room;
			} else if (cornerIndex != other.cornerIndex) {
				return cornerIndex < other.cornerIndex;
			} else if (goingUp != other.goingUp) {
				return !goingUp;
			}
			// Equality
			return false;
		}
		bool operator<=(const CornerID& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const CornerID& other) const {
			return !(*this < other);
		}
		bool operator> (const CornerID& other) const {
			return !(*this <= other);
		}

		static CornerID invalid(void) {
			return CornerID(RoomPosition::invalid(), -1, false);
		}
	};
	struct WallID {
		RoomPosition room;
		int wallIndex;

		WallID() : room(), wallIndex(-1) { }
		WallID(RoomPosition r, int i) : room(r), wallIndex(i) { }

		bool is_valid(void) const {
			return room.is_valid() && wallIndex >= 0;
		}

		bool operator== (const WallID& other) const {
			return (room == other.room && wallIndex == other.wallIndex);
		}
		bool operator!= (const WallID& other) const {
			return !(*this == other);
		}
		bool operator< (const WallID& other) const {
			if (room != other.room) {
				return room < other.room;
			}
			else if (wallIndex != other.wallIndex) {
				return wallIndex < other.wallIndex;
			}
			// Equality
			return false;
		}
		bool operator<=(const WallID& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const WallID& other) const {
			return !(*this < other);
		}
		bool operator> (const WallID& other) const {
			return !(*this <= other);
		}

		static WallID invalid(void) {
			return WallID(RoomPosition::invalid(), -1);
		}
	};
	struct NavigationNodeID {
		RoomPosition room;
		int nodeIndex;

		NavigationNodeID(RoomPosition r, int i) : room(r), nodeIndex(i) { }

		bool operator== (const NavigationNodeID& other) const {
			return (room == other.room && nodeIndex == other.nodeIndex);
		}
		bool operator!= (const NavigationNodeID& other) const {
			return (room != other.room || nodeIndex != other.nodeIndex);
		}
		bool operator< (const NavigationNodeID& other) const {
			if (room != other.room) {
				return room < other.room;
			}
			else if (nodeIndex != other.nodeIndex) {
				return nodeIndex < other.nodeIndex;
			}
			// Equality
			return false;
		}
		bool operator<=(const NavigationNodeID& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const NavigationNodeID& other) const {
			return !(*this < other);
		}
		bool operator> (const NavigationNodeID& other) const {
			return !(*this <= other);
		}
	};
	struct LineID {
		RoomPosition room;
		int lineIndex;

		LineID(void) : room(RoomPosition::invalid()), lineIndex(-1) { }
		LineID(RoomPosition r, int i) : room(r), lineIndex(i) { }

		bool is_valid(void) const {
			return room.is_valid() && lineIndex >= 0;
		}

		bool operator== (const LineID& other) const {
			return (room == other.room && lineIndex == other.lineIndex);
		}
		bool operator!= (const LineID& other) const {
			return (room != other.room || lineIndex != other.lineIndex);
		}
		bool operator< (const LineID& other) const {
			if (room != other.room) {
				return room < other.room;
			}
			else if (lineIndex != other.lineIndex) {
				return lineIndex < other.lineIndex;
			}
			// Equality
			return false;
		}
		bool operator<=(const LineID& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const LineID& other) const {
			return !(*this < other);
		}
		bool operator> (const LineID& other) const {
			return !(*this <= other);
		}

		static LineID invalid(void) {
			return LineID(RoomPosition::invalid(), -1);
		}
	};


	enum GenericIDType {
		InvalidIDType,
		CornerIDType,
		WallIDType,
		LineIDType,
	};
	union GenericIDData {
		struct EmptyStruct {} invalid;
		CornerID corner;
		WallID wall;
		LineID line;

		GenericIDData() { }
		GenericIDData(CornerID corner) : corner(corner) { }
		GenericIDData(WallID wall) : wall(wall) { }
		GenericIDData(LineID line) : line(line) { }
	};
	struct GenericID {
	private:
		GenericIDType type;
		GenericIDData data;

	public:
		GenericID() : type(InvalidIDType), data() { }
		GenericID(CornerID corner) : type(CornerIDType), data(corner) { }
		GenericID(WallID wall) : type(WallIDType), data(wall) { }
		GenericID(LineID line) : type(LineIDType), data(line) { }

		bool is_valid(void) const {
			switch (type) {
				default:
					return false;
				case CornerIDType:
					return data.corner.is_valid();
				case WallIDType:
					return data.wall.is_valid();
				case LineIDType:
					return data.line.is_valid();
			}
		}

		GenericIDType getType(void) const {
			return GenericIDType(type);
		}
		RoomPosition getRoom(void) const {
			switch (type) {
				default:
					return RoomPosition::invalid();
				case CornerIDType:
					return RoomPosition(data.corner.room);
				case WallIDType:
					return RoomPosition(data.wall.room);
				case LineIDType:
					return RoomPosition(data.line.room);
			}
		}

		bool isCorner(void) const {
			return type == CornerIDType;
		}
		bool isWall(void) const {
			return type == WallIDType;
		}
		bool isLine(void) const {
			return type == LineIDType;
		}

		CornerID& unwrapCorner(void) {
			Exceptions::assert(type == CornerIDType);
			return data.corner;
		}
		WallID& unwrapWall(void) {
			Exceptions::assert(type == WallIDType);
			return data.wall;
		}
		LineID& unwrapLine(void) {
			Exceptions::assert(type == LineIDType);
			return data.line;
		}
		const CornerID& unwrapCorner(void) const {
			Exceptions::assert(type == CornerIDType);
			return data.corner;
		}
		const WallID& unwrapWall(void) const {
			Exceptions::assert(type == WallIDType);
			return data.wall;
		}
		const LineID& unwrapLine(void) const {
			Exceptions::assert(type == LineIDType);
			return data.line;
		}


		bool operator== (const GenericID& other) const {
			if (type != other.type) {
				return false;
			}
			switch (type) {
				default:
					return true;
				case CornerIDType:
					return data.corner == other.data.corner;
				case WallIDType:
					return data.wall == other.data.wall;
				case LineIDType:
					return data.line == other.data.line;
			}
		}
		bool operator!= (const GenericID& other) const {
			return !(*this == other);
		}
		bool operator< (const GenericID& other) const {
			if (type != other.type) {
				return ((int) type) < ((int) other.type);
			}
			switch (type) {
				default:
					return false;
				case CornerIDType:
					return data.corner < other.data.corner;
				case WallIDType:
					return data.wall < other.data.wall;
				case LineIDType:
					return data.line < other.data.line;
			}
		}
		bool operator<=(const GenericID& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const GenericID& other) const {
			return !(*this < other);
		}
		bool operator> (const GenericID& other) const {
			return !(*this <= other);
		}

		static GenericID invalid(void) {
			return GenericID();
		}
	};


	enum NavigationNodeType {
		InvalidNodeType,
		StartNodeType,
		GoalNodeType,
		CornerNodeType,
	};
	struct StartNavigationNode {
		RoomPosition room;
		IntVector pos;
		bool inverseGravity;
		// TODO: more stuff

		StartNavigationNode(RoomPosition room, IntVector pos, bool inverseGravity) : room(room), pos(pos), inverseGravity(inverseGravity) { }
	};
	struct GoalNavigationNode {
		RoomPosition room;
		IntVector pos;
		// TODO: more stuff

		GoalNavigationNode(RoomPosition room, IntVector pos) : room(room), pos(pos) { }
	};
	struct CornerNavigationNode {
		CornerID corner;
		bool inverseGravity;

		CornerNavigationNode(CornerID corner, bool inverseGravity) : corner(corner), inverseGravity(inverseGravity) { }
	};
	union NavigationNodeData {
		struct EmptyStruct {} invalid;
		StartNavigationNode start;
		GoalNavigationNode goal;
		CornerNavigationNode corner;

		NavigationNodeData(StartNavigationNode start) : start(start) { }
		NavigationNodeData(GoalNavigationNode goal) : goal(goal) { }
		NavigationNodeData(CornerNavigationNode corner) : corner(corner) { }
	};
	struct NavigationNode {
		NavigationNodeType type;
		NavigationNodeData data;

		NavigationNode(NavigationNodeType type, NavigationNodeData data) : type(type), data(data) { }
		
		NavigationNode(StartNavigationNode start) : type(NavigationNodeType::StartNodeType), data(NavigationNodeData(start)) { }
		NavigationNode(GoalNavigationNode goal) : type(NavigationNodeType::GoalNodeType), data(NavigationNodeData(goal)) { }
		NavigationNode(CornerNavigationNode corner) : type(NavigationNodeType::CornerNodeType), data(NavigationNodeData(corner)) { }

		NavigationNode(CornerID corner, bool inverseGravity) : type(NavigationNodeType::CornerNodeType), data(CornerNavigationNode(corner, inverseGravity)) { }
	};
	struct NavigationEdge {
		NavigationNodeID from;
		NavigationNodeID to;
		IntVector distance;

		NavigationEdge(NavigationNodeID from, NavigationNodeID to, IntVector d) : from(from), to(to), distance(d) {}
	};

	struct RoomData {
		bool initialized;
		bool warpx, warpy;				// Does the screen wrap in x or y directions?
		bool up, down, left, right;		// Whether the respective screen edges are traversable

		std::vector<Corner> corners;
		std::vector<RoomWall> walls;
		std::vector<NavigationNode> nodes;
		std::vector<GravityLine> lines;

		RoomData() {
			initialized = warpx = warpy = up = down = left = right = false;
			corners.clear(); walls.clear(); nodes.clear(); lines.clear();
		}

		int GetMinXPos() {
			if (warpx) {
				return -9;
			}
			else {
				return -14;
			}
		}
		int GetMaxXPos() {
			if (warpx) {
				return 310;
			}
			else {
				return 307;
			}
		}
		int GetMinYPos() {
			if (warpy) {
				return -11;
			}
			else {
				return -2;
			}
		}
		int GetMaxYPos() {
			if (warpy) {
				return 226;
			}
			else {
				return 237;
			}
		}
	};

	struct SymbolicPlayerState {
		Numerics::BoolRange leftInput, rightInput;
		Numerics::BoolRange inverseGravity;
		Numerics::IntRange x, y;
		Numerics::FloatRange v_x, v_y;
	};

	struct CornerConnection {
		CornerID corner;
		bool goingUp;
		// The player state as it is the frame after fully passing the corner (second gap)
		Region pos;
		FloatInterval vx;
		FloatInterval vy;
		bool inverseGravity;
	};
	struct SurfaceConnection {
		WallID surface;
		// The player state as it is the frame when touching the surface
		Region pos;
		FloatInterval vx;
		FloatInterval vy;
		bool inverseGravity;
	};
	struct FullConnection {
		CornerConnection fromCorner;
		CornerConnection toCorner;
		std::vector<SurfaceConnection> intermediate_surfaces;
	};

	struct PlayerStateRange {
		bool inverseGravity;
		Region pos;
		FloatInterval vx;
		FloatInterval vy;

		PlayerStateRange& make_bottom(void) {
			pos.make_bottom();
			vx.make_bottom();
			vy.make_bottom();
			return *this;
		}
		PlayerStateRange& make_top(void) {
			pos.make_top();
			vx.make_top().join(FULL_X_SPEED_RANGE);
			vy.make_top().join(FULL_Y_SPEED_RANGE);
			return *this;
		}
		bool is_bottom(void) const {
			return pos.is_bottom() || vx.is_bottom() || vy.is_bottom();
		}

		bool intersects(const PlayerStateRange& other) const {
			if (is_bottom() || other.is_bottom()) {
				return false;
			} else if (inverseGravity != other.inverseGravity) {
				return false;
			} else if (!pos.intersects(other.pos)) {
				return false;
			} else if (!vx.intersects(other.vx)) {
				return false;
			} else if (!vy.intersects(other.vy)) {
				return false;
			}
			return true;
		}

		bool exactly_equals(const PlayerStateRange& other) const {
			if (is_bottom()) {
				return other.is_bottom();
			} else if (inverseGravity != other.inverseGravity) {
				return false;
			} else if (!pos.exactly_equals(other.pos)) {
				return false;
			} else if (!vx.exactly_equals(other.vx)) {
				return false;
			} else if (!vy.exactly_equals(other.vy)) {
				return false;
			}

			return true;
		}

		static PlayerStateRange bottom(void) {
			return PlayerStateRange().make_bottom();
		}
		static PlayerStateRange top(void) {
			return PlayerStateRange().make_top();
		}
	};
	struct CornerWaypoint {
		CornerID corner_id;
		PlayerStateRange playerState;

		CornerWaypoint() : corner_id(CornerID::invalid()) { }
	};
	struct SurfaceWaypoint {
		WallID wall_id;
		PlayerStateRange playerState;
		PlayerStateRange playerStateOut;

		bool doesFlip(void) const {
			if (playerState.is_bottom() || playerStateOut.is_bottom()) {
				return false;
			} else {
				return playerStateOut.inverseGravity != playerState.inverseGravity;
			}
		}
	};
	struct LineWaypoint {
		LineID line_id;
		PlayerStateRange playerState;
		PlayerStateRange playerStateOut;
	};
	struct GenericWaypoint {
		GenericID id;
		PlayerStateRange playerState;
		PlayerStateRange playerStateOut;

		GenericWaypoint() : id(), playerState() { }
		GenericWaypoint(const CornerWaypoint& corner_wp) : id(corner_wp.corner_id), playerState(corner_wp.playerState), playerStateOut(corner_wp.playerState) { }

		CornerWaypoint unwrapCornerWaypoint(void) {
			Exceptions::assert(id.isCorner());
			CornerWaypoint result;
			result.corner_id = id.unwrapCorner();
			result.playerState = playerState;
			Exceptions::assert(playerStateOut.is_bottom() || playerState.exactly_equals(playerStateOut));
			return result;
		}
		SurfaceWaypoint unwrapSurfaceWaypoint(void) {
			Exceptions::assert(id.isWall());
			SurfaceWaypoint result;
			result.wall_id = id.unwrapWall();
			result.playerState = playerState;
			result.playerStateOut = playerStateOut;
			return result;
		}
		LineWaypoint unwrapLineWaypoint(void) {
			Exceptions::assert(id.isLine());
			LineWaypoint result;
			result.line_id = id.unwrapLine();
			result.playerState = playerState;
			result.playerStateOut = playerStateOut;
			return result;
		}
	};
	struct WaypointPath {
		CornerWaypoint source;
		CornerWaypoint target;
		std::vector<GenericWaypoint> waypoints;

		WaypointPath(CornerWaypoint source) : source(source), target() {
			waypoints.clear();
		}

		bool is_partial(void) const {
			return !target.corner_id.is_valid();
		}
		bool is_bottom(void) const {
			if (source.playerState.is_bottom()) {
				return true;
			} else if (!is_partial() && target.playerState.is_bottom()) {
				return true;
			} else if (!waypoints.empty()) {
				// Only validate playerStateIn for last entry
				if (waypoints.back().playerState.is_bottom()) {
					return true;
				}
				for (std::vector<GenericWaypoint>::const_reverse_iterator wp_it = ++waypoints.crbegin(); wp_it != waypoints.crend(); wp_it++) {
					if ((*wp_it).playerState.is_bottom() || (*wp_it).playerStateOut.is_bottom()) {
						return true;
					}
				}
				return false;
			}
		}
		GenericID getLastElementID(void) const {
			if (target.corner_id.is_valid()) {
				return GenericID(target.corner_id);
			} else if (waypoints.empty()) {
				return GenericID(source.corner_id);
			} else {
				return GenericID(waypoints.back().id);
			}
		}
		PlayerStateRange& getLastPlayerState(void) {
			if (target.corner_id.is_valid()) {
				return target.playerState;
			} else if (waypoints.empty()) {
				return source.playerState;
			} else {
				return waypoints.back().playerState;
			}
		}
		const PlayerStateRange& getLastPlayerStateConst(void) const {
			if (target.corner_id.is_valid()) {
				return target.playerState;
			} else if (waypoints.empty()) {
				return source.playerState;
			} else {
				return waypoints.back().playerState;
			}
		}
	};
	struct ElementRegion {
		GenericID element_id;
		Region region;
		ElementRegion(void) : element_id(GenericID::invalid()), region(Region::bottom()) { }
		ElementRegion(GenericID element_id, Region region) : element_id(element_id), region(region) { }

		bool is_bottom(void) const {
			return region.is_bottom() || !element_id.is_valid();
		}

		bool operator== (const ElementRegion& other) const {
			if (element_id != other.element_id) {
				return false;
			} else if (region.x.getLowerBound() != other.region.x.getLowerBound()) {
				return false;
			} else if (region.x.getUpperBound() != other.region.x.getUpperBound()) {
				return false;
			} else if (region.y.getLowerBound() != other.region.y.getLowerBound()) {
				return false;
			} else if (region.y.getUpperBound() != other.region.y.getUpperBound()) {
				return false;
			}
			return true;
		}
		bool operator!= (const ElementRegion& other) const {
			return !(*this == other);
		}
		bool operator< (const ElementRegion& other) const {
			if (element_id != other.element_id) {
				return element_id < other.element_id;
			} else if (!region.x.intersects(other.region.x)) {
				return region.x < other.region.x;
			} else if (region.x.getLowerBound() == other.region.x.getLowerBound()) {
				return region.x.getUpperBound() < other.region.x.getUpperBound();
			} else if (region.x.getUpperBound() == other.region.x.getUpperBound()) {
				return region.x.getLowerBound() < other.region.x.getLowerBound();
			} else {
				// Shouldn't compare overlapping x regions of same element!
				Exceptions::error();
			}
			return false;
		}
		bool operator<=(const ElementRegion& other) const {
			return (*this < other) || (*this == other);
		}
		bool operator>=(const ElementRegion& other) const {
			return !(*this < other);
		}
		bool operator> (const ElementRegion& other) const {
			return !(*this <= other);
		}
	};
	
	// ------------------
	// Hook functions
	// ------------------
	void BeforeRenderHook(void);
	void AfterTileRenderHook(void);
	void AfterRenderHook(void);

	// -------------------
	// Rendering functions
	// -------------------
	void RenderPixel(int x, int y);
	void RenderPixel(const IntVector& pos);
	void RenderWall(WallID w);
	void RenderCorner(CornerID corner_id);
	void RenderLine(const IntVector& from, const IntVector& to);
	void RenderRegion(const Region& region);
	void RenderEdge(int edge_index);
	void RenderGravityLine(LineID line_id);
	void RenderRect(GlobalPosition min, GlobalPosition max);
	void RenderCollisionBitmap(IntVector offset);
	void RenderFullConnection(const FullConnection& conn);
	void RenderWaypointPath(const WaypointPath& path);
	void NiceRenderWaypointPath(const WaypointPath& path);

	// --------------------------------------
	// Functions
	// --------------------------------------
	RoomData& GetRoomData(RoomPosition room_pos);
	Corner& GetCorner(CornerID corner_id);
	Corner GetCornerInLocalFrame(CornerID corner_id, const LocalFrame& frame);
	RoomWall& GetWall(WallID wall_id);
	RoomWall GetWallInLocalFrame(WallID wall_id, const LocalFrame& frame);
	GravityLine& GetGravityLine(LineID line_id);
	GravityLine GetGravityLineInLocalFrame(LineID line_id, const LocalFrame& frame);
	NavigationNode& GetNavigationNode(NavigationNodeID node_id);

	bool IsSameOrInverseNode(NavigationNodeID n1, NavigationNodeID n2);
	NavigationEdge RemoveEdge(int edgeIndex);
	bool IsEdgePossibleWithoutFlipping(NavigationEdge& edge);
	bool DoEdgesCross(NavigationEdge& e1, NavigationEdge& e2);
	void RemoveElement(std::vector<int>& v, int index);
	void LoadRoom(RoomPosition room_pos);
	void InitializeConnectedRooms(RoomPosition startingRoom);
	void InitializeRoomData(RoomPosition room_pos);
	void CrossRoomInitialization(RoomPosition room_pos);
	bool CanConnectRooms(RoomPosition r1, RoomPosition r2);
	void CreateRoomNodes(RoomPosition r);
	void ConnectNodes(NavigationNodeID from, NavigationNodeID to);
	bool CanConnectCorners(CornerType c1, CornerType c2, IntVector d);
	bool CanConnectEdges(NavigationEdge& e1, NavigationEdge& e2);
	bool CanConnectCornersViaSurface(CornerID c1_id, WallID w_id, CornerID c2_id);

	bool SurfaceIsVisibleFrom(GlobalPosition sourcePos, WallID w_id);

	std::set<RoomPosition> GetTouchedRooms(GlobalPosition from, GlobalPosition to, bool y_dir);
	std::set<WallID> GetSurfacesAboveEdge(NavigationEdge& edge);
	std::set<WallID> GetSurfacesBelowEdge(NavigationEdge& edge);

	std::set<CornerID> FindCornersInRegion(GlobalPosition from, GlobalPosition to);
	std::set<CornerID> FindCornersInRelativeRegion(const LocalFrame& frame, const Region& region);
	std::set<WallID> FindWallsInRegion(GlobalPosition from, GlobalPosition to);
	std::set<WallID> FindWallsInRelativeRegion(const LocalFrame& frame, const Region& region);
	std::set<LineID> FindLinesInRegion(const GlobalPosition& from, const GlobalPosition& to);
	std::set<LineID> FindLinesInRelativeRegion(const LocalFrame& frame, const Region& region);


	std::vector<FullConnection> FindCornerConnections(CornerID c_id, bool goingUp);
	std::vector<FullConnection> RecursiveSurfaceConnections(const LocalFrame& frame, const FullConnection& history, const std::set<WallID>& surfaces, const std::set<CornerID>& corners);

	std::vector<WaypointPath> NewFindCornerConnections(CornerID c_id);
	std::vector<WaypointPath> NewRecursiveSurfaceConnections(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements);
	std::vector<ElementRegion> GetUncoveredRanges(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements);

	std::vector<WaypointPath> RevampedFindCornerConnections(CornerID c_id);
	std::vector<WaypointPath> RevampedRecursiveSurfaceConnections(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements);
	Region RevampedGetValidConnectionRegion(const LocalFrame& frame, const WaypointPath& history);
	std::vector<PlayerStateRange> RevampedGetOutgoingConnectionStates(const LocalFrame& frame, const WaypointPath& history, const std::vector<ElementRegion>& coveredRanges);
	std::vector<ElementRegion> RevampedGetCoveredRanges(const LocalFrame& frame, const WaypointPath& history);
	std::vector<ElementRegion> RevampedGetNextPossibleConnections(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements, const std::vector<ElementRegion>& coveredRanges);
	void RevampedReducePath(const LocalFrame& frame, WaypointPath& path);

	void FindConnectingSurfaces(CornerID from_id, CornerID to_id);

	void DoIntervalPhysicsStep(const FloatInterval& a_x, const FloatInterval& a_y, FloatInterval& v_x, FloatInterval& v_y, IntInterval& xp, IntInterval& yp);

	bool ReduceRangesByConnectivity(PlayerStateRange& from, PlayerStateRange& to, const LocalFrame& frame, const WallID& wall_id);

	void DoFlip(PlayerStateRange& state);
	void DoGravityLineFlip(PlayerStateRange& state);

	bool ReduceByFliplessConnectivity(Region & from, Region & to, FloatInterval v_x, FloatInterval v_y, bool inverseGravity, bool toSurface);

	void FrameAdvancePlayerStateRange(PlayerStateRange& state, const Region& blockedRegion);

	bool IsInRange(IntVector range, IntVector v);

	void BuildSurfaceConnectionGraph(NavigationNodeID from, NavigationNodeID to, bool invY);
	void VisualizeHeuristic(void);

	GlobalPosition GetNodePos(NavigationNodeID node_id);

	int GetMinXFrames(int d_x);
	int GetMaxXFrames(int d_x);
	IntInterval GetYFrames(IntInterval& d_y, bool inverseGravity);
	IntInterval GetYDist(int y_frames, bool inverseGravity);
	IntInterval GetYDist(IntInterval & y_frames, bool inverseGravity);
	int GetMinYFrames(int d_y);
	int GetMaxYFrames(int d_y);

	GlobalPosition DoAllRoomChanges(GlobalPosition& globalPos);
	GlobalPosition PlayerRoomChangeLogic(GlobalPosition& globalPos);
	IntVector ToLocalCoords(LocalFrame& localFrame, GlobalPosition& globalPos);
	GlobalPosition ToGlobalCoords(LocalFrame& localFrame, IntVector localPos);

	int PruneDeadEndEdges(void);
	int PruneDominatedEdges(void);
	int PruneBackAndCrossedEdges(void);

	// --------------------------------------
	// Getter Functions / Reading
	// --------------------------------------
	RoomPosition GetCurrentRoomPosition();
	GlobalPosition GetPlayerPosition();
	IntVector GetDistanceOffsetBetweenRooms(RoomPosition from, RoomPosition to, bool invY);
	IntVector GetMinDistanceOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	IntVector GetMinOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	IntVector GetDistanceBetween(const GlobalPosition& from, const GlobalPosition& to, bool invY);
	IntVector GetMinDistanceBetween(const GlobalPosition& from, const GlobalPosition& to);
	int GetHOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	int GetVOffsetBetweenRooms(RoomPosition from, RoomPosition to, bool invY);
	int GetMinVOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	int GetMinPositiveVOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	int GetMinNegativeVOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	uint8_t GetPlayerCollisionAt(GlobalPosition pos);
	uint8_t GetCurrentRoomPlayerCollisionAt(IntVector pos);
	uint8_t* GetCurrentRoomPlayerCollisionBitmap(IntVector min, IntVector max);

	// ------------------------
	// Raycasting Functionality
	// ------------------------
	float GlobalRaycast(RoomPosition startingRoom, Ray& ray);
	float RoomRaycast(RoomPosition room_pos, Ray& ray);
	float WallRayIntersection(RoomWall& wall, Ray& ray);
};

#endif /* SOLVER_TERRAIN_H */
