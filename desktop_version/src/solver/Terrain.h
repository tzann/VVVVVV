#ifndef TERRAIN_H
#define TERRAIN_H

#include <cstddef>
#include <vector>

#include "Exit.h"

#define VIRIDIAN_CX (6)
#define VIRIDIAN_CY (2)
#define VIRIDIAN_W (12)
#define VIRIDIAN_H (21)

#define TOWER_RX (9)

#define RENDER_OFFSET_X (11)
#define RENDER_OFFSET_Y (12)

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

	struct LocalPosition {
		int x, y;
	};
	struct IntVector {
		int x, y;

		IntVector() {
			x = 0;
			y = 0;
		}

		IntVector(int x, int y) : x(x), y(y) { }
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
			return (outside != other.outside || rx != other.rx || ry != other.ry);
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

		bool IsTower() {
			if (outside) {
				if (rx == 8) {
					// Panic Room
					return ry == 4 || ry == 5;
				}
				else if (rx == 10) {
					// Final Challenge
					return ry == 5 || ry == 6;
				}
			} else {
				// Tower
				return rx == TOWER_RX;
			}
			return false;
		}
		
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
		LocalPosition pos;
	};

	struct Ray {
		LocalPosition origin;
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
		int x, y;
		CornerType type;
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
	};
	struct WallID {
		RoomPosition room;
		int wallIndex;

		WallID(RoomPosition r, int i) : room(r), wallIndex(i) { }
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

		RoomData() {
			initialized = warpx = warpy = up = down = left = right = false;
			corners.clear(); walls.clear(); nodes.clear();
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
	void RenderEdge(int edge_index);
	void RenderCollisionBitmap(IntVector offset);

	// --------------------------------------
	// Functions
	// --------------------------------------
	RoomData& GetRoomData(RoomPosition room_pos);
	Corner& GetCorner(CornerID corner_id);
	NavigationNode& GetNavigationNode(NavigationNodeID node_id);
	bool IsSameOrInverseNode(NavigationNodeID n1, NavigationNodeID n2);
	NavigationEdge RemoveEdge(int edgeIndex);
	void RemoveElement(std::vector<int>& v, int index);
	void LoadRoom(RoomPosition room_pos);
	void InitializeConnectedRooms(RoomPosition startingRoom);
	void InitializeRoomData(RoomPosition room_pos);
	bool CanConnectRooms(RoomPosition r1, RoomPosition r2);
	void CreateRoomNodes(RoomPosition r);
	void ConnectNodes(NavigationNodeID from, NavigationNodeID to);
	bool CanConnectCorners(CornerType c1, CornerType c2, IntVector d);
	bool CanConnectEdges(NavigationEdge& e1, NavigationEdge& e2);

	int PruneDeadEndEdges(void);
	int PruneDominatedEdges(void);

	// --------------------------------------
	// Getter Functions / Reading
	// --------------------------------------
	RoomPosition GetCurrentRoomPosition();
	int GetHOffsetBetweenRooms(RoomPosition from, RoomPosition to);
	bool* GetCurrentRoomPlayerCollisionBitmap(IntVector min, IntVector max);

	// ------------------------
	// Raycasting Functionality
	// ------------------------
	float GlobalRaycast(RoomPosition startingRoom, Ray& ray);
	float RoomRaycast(RoomPosition room_pos, Ray& ray);
	float WallRayIntersection(RoomWall& wall, Ray& ray);
};

#endif /* TERRAIN_H */
