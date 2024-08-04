#ifndef SOLVER_TERRAIN_H
#define SOLVER_TERRAIN_H

#include <cstddef>
#include <vector>
#include <set>

#include "Exit.h"
#include "solver/Constants.h"
#include "solver/Geometry.h"

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
			rx = 0;
			ry = 0;
			outside = false;
		}
		RoomPosition(int rx, int ry) : rx(rx), ry(ry) {
			outside = false;
		}
		RoomPosition(bool outside, int rx, int ry) : outside(outside), rx(rx), ry(ry) { }

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
		bool simple;
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
	};

	struct GravityLine {
		bool isHorizontal;
		IntVector min;
		IntVector max;

		GravityLine() {}
		GravityLine(bool isHorizontal, IntVector min, IntVector max) : isHorizontal(isHorizontal), min(min), max(max) { }
	};

	struct CornerID {
		RoomPosition room;
		int cornerIndex;

		CornerID(RoomPosition r, int i) : room(r), cornerIndex(i) { }

		bool operator== (const CornerID& other) const {
			return (room == other.room && cornerIndex == other.cornerIndex);
		}
		bool operator!= (const CornerID& other) const {
			return (room != other.room || cornerIndex != other.cornerIndex);
		}

		bool operator< (const CornerID& other) const {
			if (room != other.room) {
				return room < other.room;
			}
			else if (cornerIndex != other.cornerIndex) {
				return cornerIndex < other.cornerIndex;
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
	};
	struct WallID {
		RoomPosition room;
		int wallIndex;

		WallID(RoomPosition r, int i) : room(r), wallIndex(i) { }

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

		LineID(RoomPosition r, int i) : room(r), lineIndex(i) { }

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
	void RenderWall(WallID w);
	void RenderCorner(CornerID corner_id);
	void RenderEdge(int edge_index);
	void RenderGravityLine(LineID line_id);
	void RenderRect(GlobalPosition min, GlobalPosition max);
	void RenderCollisionBitmap(IntVector offset);

	// --------------------------------------
	// Functions
	// --------------------------------------
	RoomData& GetRoomData(RoomPosition room_pos);
	Corner& GetCorner(CornerID corner_id);
	RoomWall& GetWall(WallID wall_id);
	GravityLine& GetGravityLine(LineID line_id);
	NavigationNode& GetNavigationNode(NavigationNodeID node_id);

	bool IsSameOrInverseNode(NavigationNodeID n1, NavigationNodeID n2);
	NavigationEdge RemoveEdge(int edgeIndex);
	bool IsEdgePossibleWithoutFlipping(NavigationEdge& edge);
	bool DoEdgesCross(NavigationEdge& e1, NavigationEdge& e2);
	void RemoveElement(std::vector<int>& v, int index);
	void LoadRoom(RoomPosition room_pos);
	void InitializeConnectedRooms(RoomPosition startingRoom);
	void InitializeRoomData(RoomPosition room_pos);
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
	std::set<WallID> FindWallsInRegion(GlobalPosition from, GlobalPosition to);

	void FindCornerConnections(CornerID c_id, bool inverseGravity);
	void FindConnectingSurfaces(CornerID from_id, CornerID to_id);

	bool IsInRange(IntVector range, IntVector v);

	void BuildSurfaceConnectionGraph(NavigationNodeID from, NavigationNodeID to, bool invY);
	void VisualizeHeuristic(void);

	GlobalPosition GetNodePos(NavigationNodeID node_id);

	int GetMinXFrames(int d_x);
	int GetMaxXFrames(int d_x);
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
	IntVector GetDistanceBetween(GlobalPosition& from, GlobalPosition& to, bool invY);
	IntVector GetMinDistanceBetween(GlobalPosition& from, GlobalPosition& to);
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
