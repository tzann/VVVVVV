#include "Terrain.h"

#include <SDL.h>

#include "Graphics.h"
#include "Map.h"
#include "Entity.h"
#include "UtilityClass.h"
#include "Game.h"
#include "Exit.h"
#include "Screen.h"

#include "solver/Heuristic.h"
#include "solver/Solver.h"


namespace Terrain {
	RoomPosition start_room = RoomPosition(2, 16);
	int start_x = 10;
	int start_y = 100;

	RoomPosition goal_room = RoomPosition(3, 4);
	int goal_x = 100;
	int goal_y = 160;

	RoomData overworldRoomData[20][20];
	RoomData outsideRoomData[20][20];
	CollisionSetting collisionSetting;
	
	// ------------------
	// Hook functions
	// ------------------

	void BeforeRenderHook(void) {

	}

	void AfterTileRenderHook(void) {

	}

	void AfterRenderHook(void) {

	}

	RoomData& GetRoomData(RoomPosition room) {
		if (room.outside) {
			if (0 <= room.rx && room.rx < 20 && 0 <= room.ry && room.ry < 20) {
				return overworldRoomData[room.rx][room.ry];
			}
		} else {
			if (0 <= room.rx && room.rx < 20 && 0 <= room.ry && room.ry < 20) {
				return outsideRoomData[room.rx][room.ry];
			}
		}

		VVV_exit(1);
	}

	void LoadRoom(RoomPosition room) {
		IntVector native_coords = room.GetNativeRoomCoords();

		if (game.roomx != native_coords.x || game.roomy != native_coords.y || map.finalmode != room.outside) {
			map.finalmode = room.outside;
			map.gotoroom(native_coords.x, native_coords.x);
		}
	}

	void InitializeRoomData(RoomPosition room_pos) {
		// Can't handle towers (yet)
		if (room_pos.IsTower()) {
			VVV_exit(-1);
			return;
		}

		RoomData& result = GetRoomData(room_pos);
		if (result.initialized) {
			// Room has already been initialized
			return;
		}
		result.initialized = true;

		// First, load the room
		LoadRoom(room_pos);

		// Now, let's first do some basic properties
		result.warpx = map.warpx;
		result.warpy = map.warpy;

		IntVector min = IntVector(result.GetMinXPos() - 1, result.GetMinYPos() - 1);
		IntVector max = IntVector(result.GetMaxXPos() + 1, result.GetMaxYPos() + 1);

		bool* collision_bitmap = GetCurrentRoomPlayerCollisionBitmap(min, max);
		int bitmap_width = max.x - min.x + 1;

		// Check if the screen edges are blocked off
		result.left = result.right = result.up = result.down = false;
		for (int x = min.x + 1; x <= max.x - 1; x++) {
			if (!collision_bitmap[min.y * bitmap_width + x]) {
				result.up = true;
			}
			if (!collision_bitmap[max.y * bitmap_width + x]) {
				result.down = true;
			}
		}
		for (int y = min.y + 1; y <= max.y - 1; y++) {
			if (!collision_bitmap[y * bitmap_width + min.x]) {
				result.up = true;
			}
			if (!collision_bitmap[y * bitmap_width + max.x]) {
				result.down = true;
			}
		}

		// Now, let's find the walls and corners
		
		// First, vertical walls
		for (int x = min.x + 1; x <= max.x; x++) {
			bool started = false;
			bool isLeftWall = false;
			bool prevIsEmpty = true;
			bool startIsConcave = false;
			int wall_start = INT_MIN;

			for (int y = min.y; y <= max.y; y++) {
				bool left = collision_bitmap[y * bitmap_width + x - 1];
				bool self = collision_bitmap[y * bitmap_width + x];

				bool isWall = left != self;
				bool newWallIsLeftWall = left && !self;

				bool wallEndsHere = started && !(isWall && newWallIsLeftWall == isLeftWall);
				bool wallStartsHere = isWall && !(started && newWallIsLeftWall == isLeftWall);

				if (wallEndsHere) {
					bool isEmpty = !left && !self;
					bool endIsConcave = !isEmpty;
					int wall_end = y;

					RoomWall newWall;
					newWall.type = isLeftWall ? WallType::LeftWall : WallType::RightWall;
					// wall.plane is the last coordinate the player can stand at just next to the wall
					newWall.plane = isLeftWall ? x : (x - 1);
					newWall.min = wall_start;
					newWall.max = wall_end;
					newWall.minCornerConcave = startIsConcave;
					newWall.maxCornerConcave = endIsConcave;

					result.walls.push_back(newWall);
					started = false;
				}
				if (wallStartsHere) {
					isLeftWall = newWallIsLeftWall;
					startIsConcave = !prevIsEmpty;
					wall_start = (y - 1);
					started = true;
				}
				prevIsEmpty = !left && !self;
			}

			// Finish off any running walls
			if (started) {
				RoomWall newWall;
				newWall.type = isLeftWall ? WallType::LeftWall : WallType::RightWall;
				// wall.plane is the last coordinate the player can stand at just next to the wall
				newWall.plane = isLeftWall ? x : (x - 1);
				newWall.min = wall_start;
				newWall.max = max.y;
				newWall.minCornerConcave = startIsConcave;
				newWall.maxCornerConcave = false;

				result.walls.push_back(newWall);
			}
		}

		// Next, horizontal walls (i.e. floors and ceilings)
		for (int y = min.y + 1; y <= max.y; y++) {
			bool started = false;
			bool isCeiling = false;
			bool prevIsEmpty = true;
			bool startIsConcave = false;
			int wall_start = INT_MIN;

			for (int x = min.x; x <= max.x; x++) {
				bool up = collision_bitmap[bitmap_width * (y - 1) + x];
				bool self = collision_bitmap[bitmap_width * y + x];

				bool isWall = up != self;
				bool newWallIsCeiling = up && !self;
				bool isSameWall = isCeiling == newWallIsCeiling;

				bool wallEndsHere = started && !(isWall && isSameWall);
				bool wallStartsHere = isWall && !(started && isSameWall);

				if (wallEndsHere) {
					bool isEmpty = !up && !self;
					bool endIsConcave = !isEmpty;
					int wall_end = x;

					RoomWall newWall;
					newWall.type = isCeiling ? WallType::Ceiling : WallType::Floor;
					// wall.plane is the last coordinate the player can stand at just next to the wall
					newWall.plane = isCeiling ? y : (y - 1);
					newWall.min = wall_start;
					newWall.max = wall_end;
					newWall.minCornerConcave = startIsConcave;
					newWall.maxCornerConcave = endIsConcave;

					result.walls.push_back(newWall);
					started = false;
				}
				if (wallStartsHere) {
					isCeiling = newWallIsCeiling;
					startIsConcave = !prevIsEmpty;
					wall_start = (x - 1);
					started = true;
				}
				prevIsEmpty = !up && !self;
			}

			// Finish off any running walls
			if (started) {
				RoomWall newWall;
				newWall.type = isCeiling ? WallType::Ceiling : WallType::Floor;
				// wall.plane is the last coordinate the player can stand at just next to the wall
				newWall.plane = isCeiling ? y : (y - 1);
				newWall.min = wall_start;
				newWall.max = max.x;
				newWall.minCornerConcave = startIsConcave;
				newWall.maxCornerConcave = false;

				result.walls.push_back(newWall);
			}
		}

		// Now we can gather the (convex) corners
		for (int x = min.x + 1; x <= max.x; x++) {
			for (int y = min.y + 1; y <= max.y; y++) {
				bool up_left = collision_bitmap[bitmap_width * (y - 1) + x - 1];
				bool left = collision_bitmap[y * bitmap_width + x - 1];
				bool up = collision_bitmap[bitmap_width * (y - 1) + x];
				bool self = collision_bitmap[bitmap_width * y + x];

				int wall_count = up_left + left + up + self;
				if (wall_count != 1) {
					// Not a convex corner
					continue;
				}

				Corner newCorner;
				if (up_left) {
					newCorner.type = BottomRight;
					newCorner.x = x;
					newCorner.y = y;
				} else if (left) {
					newCorner.type = TopRight;
					newCorner.x = x;
					newCorner.y = y - 1;
				} else if (up) {
					newCorner.type = BottomLeft;
					newCorner.x = x - 1;
					newCorner.y = y;
				} else {
					newCorner.type = TopLeft;
					newCorner.x = x - 1;
					newCorner.y = y - 1;
				}

				result.corners.push_back(newCorner);
			}
		}

		// Free the collision bitmap
		SDL_free((void*) collision_bitmap);
	}

	bool CanConnectRooms(RoomPosition r1, RoomPosition r2) {
		if (r1.outside != r2.outside) {
			// different dimensions
			return false;
		}
		if (r1.rx == r2.rx && r1.ry == r2.ry) {
			// same room
			return true;
		}
		if (r1.IsTower() || r2.IsTower()) {
			// TODO: implement this
			VVV_exit(-1);
			return true;
		}

		RoomData& r1Data = GetRoomData(r1);
		RoomData& r2Data = GetRoomData(r2);
		if (!r1Data.initialized || !r2Data.initialized) {
			// Room data hasn't been initialized yet!
			VVV_exit(1);
			return false;
		}

		bool canExitH = false;
		bool canExitUp = false;
		bool canExitDown = false;
		bool canEnterH = false;
		bool canEnterUp = false;
		bool canEnterDown = false;
		if (r1.rx != r2.rx) {
			canEnterH = canExitH = true;

			bool goingRight = r2.rx > r1.rx;
			if (r1.outside && (r1.rx < TOWER_RX) != (r2.rx < TOWER_RX)) {
				// Wrap around because tower is in the way
				goingRight = r2.rx < TOWER_RX;
			}
			if (goingRight) {
				canExitH = r1Data.right;
				canEnterH = r2Data.left;
			} else {
				canExitH = r1Data.left;
				canEnterH = r2Data.right;
			}
		}

		canExitUp = r1Data.up;
		canExitDown = r1Data.down;
		canEnterUp = r2Data.up;
		canEnterDown = r2Data.down;

		if (canEnterUp && (canExitH || canExitDown)) {
			return true;
		}
		if (canEnterDown && (canExitH || canExitUp)) {
			return true;
		}
		if (canEnterH && (canExitUp || canExitH || canExitDown)) {
			return true;
		}

		return false;
	}

	void ConnectCorners(RoomPosition room_i, int i, RoomPosition room_j, int j) {
		RoomData& roomData1 = GetRoomData(room_i);
		RoomData& roomData2 = GetRoomData(room_j);

		Corner& c1 = roomData1.corners.at(i);
		Corner& c2 = roomData2.corners.at(j);

		int d_rx = GetHOffsetBetweenRooms(room_i, room_j);

		int d_ry1 = room_j.ry - room_i.ry;
		int d_ry2, d_ry3 = INT_MAX;
		
		if (d_ry1 >= 0) {
			d_ry2 = d_ry1 - 20;
		}
		if (d_ry1 <= 0) {
			d_ry3 = d_ry1 + 20;
		}

		// Sort by distance
		if (std::abs(d_ry2) < std::abs(d_ry1)) {
			int tmp = d_ry1;
			d_ry1 = d_ry2;
			d_ry2 = tmp;
		}
		if (std::abs(d_ry3) < std::abs(d_ry2)) {
			int tmp = d_ry2;
			d_ry2 = d_ry3;
			d_ry3 = tmp;
		}
		if (std::abs(d_ry2) < std::abs(d_ry1)) {
			int tmp = d_ry1;
			d_ry1 = d_ry2;
			d_ry2 = tmp;
		}

		std::vector<IntVector> ds;
		if (d_ry1 < INT_MAX) {
			ds.emplace_back(d_rx, d_ry1);
		}
		if (d_ry2 < INT_MAX) {
			ds.emplace_back(d_rx, d_ry2);
		}
		if (d_ry3 < INT_MAX) {
			ds.emplace_back(d_rx, d_ry3);
		}

		for (int d_idx = 0; d_idx < ds.size(); d_idx++) {
			IntVector room_delta = ds.at(d_idx);
			int dx = c2.x - c1.x + room_delta.x * 320;
			int dy = c2.y - c1.y + room_delta.y * 240;

			// Check if the corners are compatible
			if (!CanConnectCorners(c1.type, c2.type, IntVector(dx, dy))) {
				continue;
			}

			// Gravity: false -> down, true -> up
			bool c1Gravity = (dy < 0);
			bool c2Gravity = (dy < 0);
			if (dy == 0) {
				if (c1.type == CornerType::TopLeft || c1.type == TopRight) {
					c1Gravity = true;
				}
				if (c2.type == CornerType::BottomLeft || c2.type == BottomRight) {
					c2Gravity = true;
				}
			}

			// TODO: clean this up
			CornerID c1_id;
			c1_id.room = room_i;
			c1_id.cornerIndex = i;
			NavigationNodeUnion c1_node_union = {};
			c1_node_union.corner = c1_id;
			NavigationNode c1_node = {};
			c1_node.data = c1_node_union;
			c1_node.type = NavigationNodeType::CornerNodeType;
			c1_node.gravity = c1Gravity;

			Ray ray = Ray(c1.x, c2.x, dx, dy);
			float t = GlobalRaycast(room_i, ray);
			if (t >= 1) {
				// No intersection found between the corners
				// TODO: connect them

				break;
			} else if (t < 0) {
				// Something weird happened
				VVV_exit(-1);
				return;
			}
		}
	}

	bool CanConnectCorners(CornerType c1, CornerType c2, IntVector d) {
		if (d.y == 0) {
			if (d.x == 0) {
				return false;
			} else if (d.x > 0) {
				if (c1 == CornerType::BottomRight || c1 == CornerType::TopRight) {
					return false;
				} else if (c2 == CornerType::BottomLeft || c2 == CornerType::TopLeft) {
					return false;
				}
			} else {
				// d.x < 0
				if (c1 == CornerType::BottomLeft || c1 == CornerType::TopLeft) {
					return false;
				} else if (c2 == CornerType::BottomRight || c2 == CornerType::TopRight) {
					return false;
				}
			}
		} else if (d.y > 0) {
			if (d.x == 0) {
				if (c1 == CornerType::BottomLeft || c1 == CornerType::BottomRight) {
					return false;
				} else if (c2 == CornerType::TopLeft || c2 == CornerType::TopRight) {
					return false;
				}
			} else if (d.x > 0) {
				// Going down and right
				if (c1 == CornerType::BottomRight || c1 == CornerType::TopLeft) {
					return false;
				} else if (c2 == CornerType::BottomRight || c2 == CornerType::TopLeft) {
					return false;
				}
			} else {
				// d.x < 0
				// Going down and left
				if (c1 == CornerType::BottomLeft || c1 == CornerType::TopRight) {
					return false;
				} else if (c2 == CornerType::BottomLeft || c2 == CornerType::TopRight) {
					return false;
				}
			}
		} else {
			// d.y < 0
			if (d.x == 0) {
				if (c1 == CornerType::TopLeft || c1 == CornerType::TopRight) {
					return false;
				} else if (c2 == CornerType::BottomLeft || c2 == CornerType::BottomRight) {
					return false;
				}
			} else if (d.x > 0) {
				// Going up and right
				if (c1 == CornerType::BottomLeft || c1 == CornerType::TopRight) {
					return false;
				} else if (c2 == CornerType::BottomLeft || c2 == CornerType::TopRight) {
					return false;
				}
			} else {
				// d.x < 0
				// Going up and left
				if (c1 == CornerType::BottomRight || c1 == CornerType::TopLeft) {
					return false;
				} else if (c2 == CornerType::BottomRight || c2 == CornerType::TopLeft) {
					return false;
				}
			}
		}

		return true;
	}

	// --------------------------------------
	// Getter Functions
	// --------------------------------------
	RoomPosition GetCurrentRoomPosition() {
		return RoomPosition::FromNativeRoomCoords(game.roomx, game.roomy);
	}

	int GetHOffsetBetweenRooms(RoomPosition from, RoomPosition to) {
		int d_rx = to.rx - from.rx;
		if ((from.rx < TOWER_RX) != (to.rx < TOWER_RX)) {
			if (from.rx < TOWER_RX) {
				d_rx -= 20;
			}
			else {
				d_rx += 20;
			}
		}

		return d_rx;
	}

	bool* GetCurrentRoomPlayerCollisionBitmap(IntVector min, IntVector max) {
		int x_extent = max.x - min.x + 1;
		int y_extent = max.y - min.y + 1;

		bool* bitmap = (bool*) SDL_malloc(x_extent * y_extent * sizeof(bool));

		for (int x = min.x; x <= max.x; x++) {
			for (int y = min.y; y <= max.y; y++) {
				// Player hitbox
				const SDL_Rect temprect = { x + VIRIDIAN_CX, y + VIRIDIAN_CY, VIRIDIAN_W, VIRIDIAN_H };

				bool collision = false;
				// Check walls
				if (collisionSetting == CollisionSetting::Walls || collisionSetting == CollisionSetting::WallsAndSpikes) {
					if (obj.checkwall(false, temprect)) {
						collision = true;
					}
				}
				// Check spikes
				if (collisionSetting == CollisionSetting::WallsAndSpikes) {
					for (size_t j = 0; j < obj.blocks.size(); j++) {
						if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect)) {
							collision = true;
						}
					}
				}

				// Store result
				bitmap[y * x_extent + x] = collision;
			}
		}

		return bitmap;
	}

	// ------------------------
	// Raycasting Functionality
	// ------------------------
	float GlobalRaycast(RoomPosition startingRoom, Ray& ray) {
		RoomPosition currentRoom = startingRoom;
		Ray currentRay = ray;
		while (true) {
			RoomData& room_data = GetRoomData(currentRoom);

			// First, check if the ray can even enter the room
			IntVector min = IntVector(room_data.GetMinXPos(), room_data.GetMinYPos());
			IntVector max = IntVector(room_data.GetMaxXPos(), room_data.GetMaxYPos());

			// AABB intersection
			float t_left = ((float)(min.x - ray.origin.x)) / ((float)ray.direction.x);
			float t_right = ((float)(max.x - ray.origin.x)) / ((float)ray.direction.x);
			float t_top = ((float)(min.y - ray.origin.y)) / ((float)ray.direction.y);
			float t_bottom = ((float)(max.y - ray.origin.y)) / ((float)ray.direction.y);

			float t_x_min, t_x_max, t_y_min, t_y_max;
			if (t_left < t_right) {
				t_x_min = t_left;
				t_x_max = t_right;
			} else {
				t_x_min = t_right;
				t_x_max = t_left;
			}
			if (t_top < t_bottom) {
				t_y_min = t_top;
				t_y_max = t_bottom;
			} else {
				t_y_min = t_bottom;
				t_y_max = t_top;
			}

			float t_min = SDL_max(t_x_min, t_y_min);
			float t_max = SDL_min(t_x_max, t_y_max);

			if (t_min >= t_max) {
				// Ray never enters room
				VVV_exit(1);
				return INFINITY;
			}

			bool rayStartsInRoom = t_min <= 0;
			bool rayEndsInRoom = t_max > 1;

			// Check if the side the ray enters from is traversable
			if (!rayStartsInRoom) {
				if (!room_data.up && t_top == t_min) {
					return t_min;
				}
				if (!room_data.down && t_bottom == t_min) {
					return t_min;
				}
				if (!room_data.left && t_left == t_min) {
					return t_min;
				}
				if (!room_data.right && t_right == t_min) {
					return t_min;
				}
			}

			float room_result = RoomRaycast(currentRoom, ray);
			if (0 <= room_result && room_result < INFINITY) {
				return room_result;
			}

			// Failsafe: if the exiting side is not traversable, stop the ray at the exiting side
			// TODO: Could happen with warping rooms for example, not sure if this is the right approach
			if (!rayEndsInRoom) {
				if (t_top == t_max) {
					// Ray exits top edge of screen
					if (room_data.up) {
						currentRoom.ry--;
					} else {
						return t_max;
					}
				}
				if (t_bottom == t_max) {
					if (room_data.down) {
						// Ray exits bottom edge of screen
						currentRoom.ry++;
					} else {
						return t_max;
					}
				}
				if (t_left == t_max) {
					if (room_data.left) {
						// Ray exits left edge of screen
						currentRoom.rx--;
					} else {
						return t_max;
					}
				}
				if (t_right == t_max) {
					if (room_data.right) {
						// Ray exits right edge of screen
						currentRoom.rx++;
					} else {
						return t_max;
					}
				}
			} else {
				// Reached end of ray, no intersection
				return INFINITY;
			}
		}
	}

	float RoomRaycast(RoomPosition room_pos, Ray& ray) {
		RoomData& room_data = GetRoomData(room_pos);
		
		// First, check if the ray can even enter the room
		IntVector min = IntVector(room_data.GetMinXPos(), room_data.GetMinYPos());
		IntVector max = IntVector(room_data.GetMaxXPos(), room_data.GetMaxYPos());

		// AABB intersection
		float t_left = ((float)(min.x - ray.origin.x)) / ((float)ray.direction.x);
		float t_right = ((float)(max.x - ray.origin.x)) / ((float)ray.direction.x);
		float t_top = ((float)(min.y - ray.origin.y)) / ((float)ray.direction.y);
		float t_bottom = ((float)(max.y - ray.origin.y)) / ((float)ray.direction.y);
		
		float t_x_min, t_x_max, t_y_min, t_y_max;
		if (t_left < t_right) {
			t_x_min = t_left;
			t_x_max = t_right;
		} else {
			t_x_min = t_right;
			t_x_max = t_left;
		}
		if (t_top < t_bottom) {
			t_y_min = t_top;
			t_y_max = t_bottom;
		} else {
			t_y_min = t_bottom;
			t_y_max = t_top;
		}

		float t_min = SDL_max(t_x_min, t_y_min);
		float t_max = SDL_min(t_x_max, t_y_max);

		if (t_min >= t_max) {
			// Ray never enters room
			return INFINITY;
		}

		bool rayStartsInRoom = t_min <= 0;
		bool rayEndsInRoom = t_max > 1;

		// Check if the side the ray enters from is traversable
		if (!rayStartsInRoom) {
			if (!room_data.up && t_top == t_min) {
				return t_min;
			}
			if (!room_data.down && t_bottom == t_min) {
				return t_min;
			}
			if (!room_data.left && t_left == t_min) {
				return t_min;
			}
			if (!room_data.right && t_right == t_min) {
				return t_min;
			}
		}

		// Wall Intersection tests
		int num_walls = room_data.walls.size();
		float min_t = INFINITY;
		for (int w = 0; w < num_walls; w++) {
			RoomWall& wall = room_data.walls.at(w);

			// Wall intersections, but keep the lowest result
			// TODO: if we only care about binary result, this is inefficient
			float result = WallRayIntersection(wall, ray);
			if (result < min_t) {
				min_t = result;
			}
		}
		if (min_t <= INFINITY) {
			return min_t;
		}

		// Failsafe: if the exiting side is not traversable, stop the ray at the exiting side
		// TODO: Could happen with warping rooms for example, not sure if this is the right approach
		if (!rayEndsInRoom) {
			if (!room_data.up && t_top == t_max) {
				return t_max;
			}
			if (!room_data.down && t_bottom == t_max) {
				return t_max;
			}
			if (!room_data.left && t_left == t_max) {
				return t_max;
			}
			if (!room_data.right && t_right == t_max) {
				return t_max;
			}
		}

		return INFINITY;
	}

	float WallRayIntersection(RoomWall& wall, Ray& ray) {
		float t, coord;
		int minCmp, maxCmp;

		bool isHorizontal = wall.type == WallType::Ceiling || wall.type == WallType::Floor;
		if (isHorizontal) {
			// Horizontal surface
			if (ray.direction.y == 0) {
				// Never intersect parallel rays
				return INFINITY;
			} else {
				t = ((float)(wall.plane - ray.origin.y)) / ((float)ray.direction.y);
				coord = ((float) ray.origin.x) + ((float)ray.direction.x) * t;

				// Does the ray exactly intersect the minCorner?
				int x1 = wall.min - ray.origin.x;
				int x2 = ray.direction.x - x1;
				int y1 = wall.plane - ray.origin.y;
				int y2 = ray.direction.y - y1;
				minCmp = x1 * y2 - x2 * y1;
				// Does the ray exactly intersect the maxCorner?
				x1 = wall.max - ray.origin.x;
				x2 = ray.direction.x - x1;
				maxCmp = x1 * y2 - x2 * y1;
			}
		} else {
			// Vertical wall
			if (ray.direction.x == 0) {
				// Never intersect parallel rays
				return INFINITY;
			} else {
				// Vertical wall
				t = ((float)(wall.plane - ray.origin.x)) / ((float)ray.direction.x);
				coord = ray.origin.x + ray.direction.y * t;

				// Does the ray exactly intersect the minCorner?
				int x1 = wall.plane - ray.origin.x;
				int x2 = ray.direction.x - x1;
				int y1 = wall.min - ray.origin.y;
				int y2 = ray.direction.y - y1;
				minCmp = x1 * y2 - x2 * y1;
				// Does the ray exactly intersect the maxCorner?
				y1 = wall.max - ray.origin.y;
				y2 = ray.direction.y - y1;
				maxCmp = x1 * y2 - x2 * y1;
			}
		}

		if (maxCmp <= minCmp) {
			VVV_exit(1);
			return INFINITY;
		}

		if (minCmp < 0 || maxCmp > 0) {
			// Ray intersects plane before minCorner
			return INFINITY;
		} else if (minCmp > 0 && maxCmp < 0) {
			// Ray intersects between minCorner and maxCorner -> intersection!
			return t;
		}
		else if (maxCmp == 0) {
			// Ray exactly intersects maxCorner
			if (wall.maxCornerConcave) {
				return t;
			} else if (ray.direction.x < 0 && ray.direction.y > 0 && wall.type == WallType::Floor) {
				return t;
			} else if (ray.direction.x > 0 && ray.direction.y < 0 && wall.type == WallType::RightWall) {
				return t;
			} else if (ray.direction.x < 0 && ray.direction.y < 0 && (wall.type == WallType::Ceiling || wall.type == WallType::LeftWall)) {
				return t;
			} else {
				// Ray points away from or parallel to a concave corner, no intersection
				return INFINITY;
			}
		} else if (minCmp == 0) {
			// Ray exactly intersects minCorner
			if (wall.minCornerConcave) {
				return t;
			} else if (ray.direction.x < 0 && ray.direction.y > 0 && wall.type == WallType::LeftWall) {
				return t;
			} else if (ray.direction.x > 0 && ray.direction.y < 0 && wall.type == WallType::Ceiling) {
				return t;
			} else if (ray.direction.x > 0 && ray.direction.y > 0 && (wall.type == WallType::Floor || wall.type == WallType::RightWall)) {
				return t;
			} else {
				// Ray points away from or parallel to a concave corner, no intersection
				return INFINITY;
			}
		} else {
			// Something weird happened (NaN?)
			VVV_exit(1);
		}

		// Failsafe: no intersection
		return INFINITY;
	}
}
