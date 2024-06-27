#ifndef TERRAIN_H
#define TERRAIN_H

#include <cstddef>
#include <vector>

#define VIRIDIAN_CX (6)
#define VIRIDIAN_CY (2)
#define VIRIDIAN_W (12)
#define VIRIDIAN_H (21)

#define RENDER_OFFSET_X (11)
#define RENDER_OFFSET_Y (12)

namespace Terrain {
	static int colors[12][3] = {
		{ 0xff, 0x00, 0x00 }, { 0xff, 0x7f, 0x00 }, { 0xff, 0xff, 0x00 }, { 0x7f, 0xff, 0x00 },
		{ 0x00, 0xff, 0x00 }, { 0x00, 0xff, 0x7f }, { 0x00, 0xff, 0xff }, { 0x00, 0x7f, 0xff },
		{ 0x00, 0x00, 0xff }, { 0x7f, 0x00, 0xff }, { 0xff, 0x00, 0xff }, { 0xff, 0x00, 0x7f },
	};

	struct OldCornerStruct {
		int x;
		int y;
		int mask;

		OldCornerStruct(int a, int b, int c) : x(a), y(b), mask(c) {}
	};

	enum CollisionKind {
		None,
		Walls,
		WallsAndSpikes,
	};

	enum NavCornerType {
		InvalidCorner,
		BottomRight, // TL is full, rest air
		BottomLeft,  // TR is full, rest air
		TopLeft,	 // BR is full, rest air
		TopRight,	 // BL is full, rest air
		ConcaveTL,	 // TL is air, rest full
		ConcaveTR,   // TR is air, rest full
		ConcaveBR,   // BR is air, rest full
		ConcaveBL,   // BL is air, rest full
		QuadTLBR,
		QuadBLTR,
	};

	struct NavEdge {
		int target; // Index of destination corner
		int dx, dy; // Horizontal and vertical distance
		bool active;
	};

	struct NavCorner {
		NavCornerType type;
		int x, y;
		int room_x, room_y;
		std::vector<NavEdge> edges; // Connections to other corners

		NavCorner(NavCornerType type, int rx, int ry, int x, int y) : type(type), x(x), y(y), room_x(rx), room_y(ry) {
			this->edges.clear();
		}

		bool hasUpEdge() {
			switch (this->type) {
			case BottomRight:
			case BottomLeft:
			case ConcaveTL:
			case ConcaveTR:
			case QuadTLBR:
			case QuadBLTR:
				return true;
			default:
				return false;
			}
		}
		bool hasDownEdge() {
			switch (this->type) {
			case TopRight:
			case TopLeft:
			case ConcaveBL:
			case ConcaveBR:
			case QuadTLBR:
			case QuadBLTR:
				return true;
			default:
				return false;
			}
		}
		bool hasLeftEdge() {
			switch (this->type) {
			case TopRight:
			case BottomRight:
			case ConcaveTL:
			case ConcaveBL:
			case QuadTLBR:
			case QuadBLTR:
				return true;
			default:
				return false;
			}
		}
		bool hasRightEdge() {
			switch (this->type) {
			case TopLeft:
			case BottomLeft:
			case ConcaveTR:
			case ConcaveBR:
			case QuadTLBR:
			case QuadBLTR:
				return true;
			default:
				return false;
			}
		}

		bool sharesEdgeWith(NavCorner& other) {
			// Only checks if the corners *could* share an edge, not that they actually do
			if (other.x == this->x && other.room_x == this->room_x) {
				// Vertically aligned
				if (other.room_y > this->room_y || (other.room_y == this->room_y && other.y > this->y)) {
					// Other corner is below
					return this->hasDownEdge() && other.hasUpEdge();
				}
				else if (other.room_y < this->room_y || (other.room_y == this->room_y && other.y < this->y)) {
					// Other corner is above
					return this->hasUpEdge() && other.hasDownEdge();
				}
			}
			else if (other.y == this->y && other.room_y == this->room_y) {
				// Horizontally aligned
				if (other.room_x > this->room_x || (other.room_x == this->room_x && other.x > this->x)) {
					// Other corner is to the right
					return this->hasRightEdge() && other.hasLeftEdge();
				}
				else if (other.room_x < this->room_x || (other.room_x == this->room_x && other.x < this->x)) {
					// Other corner is to the left
					return this->hasLeftEdge() && other.hasRightEdge();
				}
			}

			return false;
		}
	};

	enum WallOrientation {
		InvalidWall,
		Up,		// Floor
		Right,
		Down,	// Ceiling
		Left,
	};

	struct Wall {
		WallOrientation orientation;
		int x, y;	// Position of wall's left corner (from the perspective of the wall)
		int room_x, room_y;
		int width;	// Total width of wall

		Wall(WallOrientation orientation, int x, int y, int rx, int ry, int width) : orientation(orientation), x(x), y(y), room_x(rx), room_y(ry), width(width) { }

		bool RayIntersect(int ox, int oy, int dx, int dy) {
			if (orientation == Up || orientation == Down) {
				int min_y, max_y;
				if (dy == 0) {
					// Ray is parallel to wall
					return false;
				}
				else if (dy > 0) {
					min_y = oy;
					max_y = oy + dy;
				}
				else {
					min_y = oy + dy;
					max_y = oy;
				}

				if ((min_y <= y && max_y <= y) || (min_y >= y && max_y >= y)) {
					// Ray doesn't cross extended wall
					return false;
				}
				else if (dx == 0 && (ox == x || ox == x + (orientation == Down ? -width : width))) {
					// Ray exactly touches the end of the wall, let it through
					return false;
				}

				float t = ((float)(y - oy)) / ((float)dy);
				// we can assume 0 < t < 1 because of min/max checks
				float i_x = ((float)ox) + ((float)dx) * t;

				float x_dist = orientation == Down ? x - i_x : i_x - x;
				// Intersection is within wall's range
				return 0 < x_dist && x_dist < width;
			} else {
				// assume(orientation == Right || Orientation == Left)
				int min_x, max_x;
				if (dx == 0) {
					return false;
				}
				else if (dx > 0) {
					min_x = ox;
					max_x = ox + dx;
				}
				else {
					min_x = ox + dx;
					max_x = ox;
				}

				if ((min_x <= x && max_x <= x) || (min_x >= x && max_x >= x)) {
					// Ray doesn't cross extended wall
					return false;
				}
				else if (dy == 0 && (oy == y || oy == y + (orientation == Left ? width : -width))) {
					// Ray exactly touches the end of the wall, let it through
					return false;
				}

				float t = ((float)(x - ox)) / ((float)dx);
				float i_y = ((float)oy) + ((float)dy) * t;

				float y_dist = orientation == Left ? y - i_y : i_y - y;
				// Intersection is within wall's range
				return 0 < y_dist && y_dist < width;
			}
		}
	};

	struct AABB {
		int x_min;
		int x_max;
		int y_min;
		int y_max;

		AABB(int a, int b, int c, int d) : x_min(a), x_max(b), y_min(c), y_max(d) {}

		bool RayIntersect(int ox, int oy, int dx, int dy) {
			// Don't intersect parallel rays
			if (dx == 0) {
				if (x_min == x_max - 1) {
					return false;
				}
				else if (x_min == ox || x_max == ox) {
					return false;
				}
			}
			else if (dy == 0) {
				if (y_min == y_max - 1) {
					return false;
				}
				else if (y_min == oy || y_max == oy) {
					return false;
				}
			}


			float t_x_min = ((float)(x_min - ox)) / ((float)dx);
			float t_x_max = ((float)(x_max - ox)) / ((float)dx);
			float t_y_min = ((float)(y_min - oy)) / ((float)dy);
			float t_y_max = ((float)(y_max - oy)) / ((float)dy);

			float t_min = std::max(std::min(t_x_min, t_x_max), std::min(t_y_min, t_y_max));
			float t_max = std::min(std::max(t_x_min, t_x_max), std::max(t_y_min, t_y_max));

			return 0 < t_max && t_min < 1 && t_min <= t_max;
		}
	};

	namespace RoomInfoState {
		enum RoomInfoState {
			None,
			Dimensions,
			Walls,
			NavigationGraph,
		};
	}

	struct RoomCoords {
		int rx;
		int ry;

		RoomCoords(int id) {
			rx = id >> 8;
			ry = id & 0xff;
		}

		RoomCoords(int rx, int ry) : rx(rx), ry(ry) { }

		int get_id(void) {
			return (rx << 8) | ry;
		}

		RoomCoords next_above(void) {
			int new_ry = (ry == 0) ? 19 : (ry - 1);
			return RoomCoords(rx, new_ry);
		}
		RoomCoords next_below(void) {
			int new_ry = (ry == 19) ? 0 : (ry + 1);
			return RoomCoords(rx, new_ry);
		}
		RoomCoords next_left(void) {
			int new_rx = (rx == 0) ? 19 : (rx - 1);
			return RoomCoords(new_rx, ry);
		}
		RoomCoords next_right(void) {
			int new_rx = (rx == 19) ? 0 : (rx + 1);
			return RoomCoords(new_rx, ry);
		}
	};

	struct RoomInfo {
		RoomInfoState::RoomInfoState state;
		int min_x_pos, max_x_pos, min_y_pos, max_y_pos;
		bool warpx, warpy, towermode;

		RoomInfo() {
			state = RoomInfoState::None;
			min_x_pos = min_y_pos = INT_MAX;
			max_x_pos = max_y_pos = INT_MIN;
			warpx = warpy = towermode = false;
		}

		RoomInfo(int min_x, int max_x, int min_y, int max_y) : min_x_pos(min_x), max_x_pos(max_x), min_y_pos(min_y), max_y_pos(max_y) {
			state = RoomInfoState::Dimensions;
			warpx = warpy = towermode = false;
		}

		RoomInfo(int min_x, int max_x, int min_y, int max_y, bool pre, bool warp_x, bool warp_y, bool tower) : min_x_pos(min_x), max_x_pos(max_x), min_y_pos(min_y), max_y_pos(max_y), warpx(warp_x), warpy(warp_y), towermode(tower) {
			state = RoomInfoState::Dimensions;
		}
	};

	bool IsConvexCorner(NavCornerType type);
	void ResetState(void);
	void LoadRoom(RoomCoords room);

	RoomInfoState::RoomInfoState GetRoomState(RoomCoords room_coords);
	void PrecomputeCurrentRoomWalls(void);
	void PrecomputeCurrentRoomDims(void);
	void PrecomputeNavigationGraph(void);
	bool CheckPlayerCollisionCurrentRoom(int x, int y);

	void FullPrecomputation(RoomCoords start);
	void Precompute(void);

	bool IsDirectionCompatible(NavCorner& corner, int dx, int dy);
	bool CrossRoomRayCast(int o_rx, int o_ry, int o_x, int o_y, int d_x, int d_y);
	bool TryConnectCorners(NavCorner& source, NavCorner& target);

	void BeforeRenderHook(void);
	void AfterTileRenderHook(void);
	void AfterRenderHook(void);

	void RenderWall(Wall& w);
	void RenderCorner(NavCorner& c);
	void RenderEdge(NavCorner& c, NavEdge& e);
	void RenderPixel(int x, int y);

	void ResetState();
	void LoadRoom(RoomCoords room_coords);
	RoomCoords GetCurrentRoomCoords(void);

	bool CheckWall(int x, int y);
	bool CheckSpike(int x, int y);
	bool CheckPlayerCollision(int x, int y);
	bool CheckPlayerSpike(int x, int y);

}

#endif /* TERRAIN_H */
