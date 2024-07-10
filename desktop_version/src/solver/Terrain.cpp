#include "Terrain.h"

#include <algorithm>
#include <iterator>
#include <set>
#include <unordered_set>
#include <functional>
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
	int start_x = 191;
	int start_y = 33;
	bool start_gravity = 0;

	RoomPosition goal_room = RoomPosition(4, 4);
	int goal_x = 10;
	int goal_y = 170;

	RoomData overworldRoomData[20][20];
	RoomData outsideRoomData[20][20];
	CollisionSetting collisionSetting = CollisionSetting::Walls;

	std::vector<NavigationEdge> edges;
		
	// ------------------
	// Hook functions
	// ------------------
	void BeforeRenderHook(void) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		
		if (!GetRoomData(currentRoom).initialized) {
			InitializeConnectedRooms(currentRoom);
			for (int rx = 0; rx < 20; rx++) {
				for (int ry = 0; ry < 20; ry++) {
					RoomPosition room(currentRoom.outside, rx, ry);
					if (GetRoomData(room).initialized) {
						CreateRoomNodes(room);
					}
				}
			}

			RoomData& currentRoomData = GetRoomData(currentRoom);
			int num_nodes = currentRoomData.nodes.size();
			// Connect nodes between rooms
			for (int r1 = 0; r1 < 400; r1++) {
				RoomPosition room_1(r1 % 20, r1 / 20);
				RoomData& room_1_data = GetRoomData(room_1);
				if (!room_1_data.initialized) {
					continue;
				}
				int num_r1_nodes = room_1_data.nodes.size();
				if (num_r1_nodes == 0) {
					continue;
				}

				for (int i = 0; i < num_r1_nodes; i++) {
					for (int j = i + 1; j < num_r1_nodes; j++) {
						NavigationNodeID n1(room_1, i);
						NavigationNodeID n2(room_1, j);
						ConnectNodes(n1, n2);
						ConnectNodes(n2, n1);
					}
				}

				for (int r2 = r1 + 1; r2 < 400; r2++) {
					RoomPosition room_2(r2 % 20, r2 / 20);

					RoomData& room_2_data = GetRoomData(room_2);
					if (!room_2_data.initialized) {
						continue;
					}

					if (!CanConnectRooms(room_1, room_2)) {
						continue;
					}

					int num_r2_nodes = room_2_data.nodes.size();
					if (num_r2_nodes == 0) {
						continue;
					}

					for (int i = 0; i < num_r1_nodes; i++) {
						for (int j = 0; j < num_r2_nodes; j++) {
							NavigationNodeID n1(room_1, i);
							NavigationNodeID n2(room_2, j);
							ConnectNodes(n1, n2);
							ConnectNodes(n2, n1);
						}
					}
				}
			}

			int totalPruned = 0;
			while (true) {
				int numPruned = 0;
				numPruned += PruneDeadEndEdges();
				numPruned += PruneDominatedEdges();
				if (numPruned == 0) {
					break;
				} else {
					totalPruned += numPruned;
				}
			}

			LoadRoom(goal_room);
		}
	}

	void AfterTileRenderHook(void) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		RoomData& currentRoomData = GetRoomData(currentRoom);

		if (currentRoomData.initialized) {
			SDL_SetRenderDrawBlendMode(gameScreen.m_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
			int num_walls = currentRoomData.walls.size();
			for (int w = 0; w < num_walls; w++) {
				WallID w_id(currentRoom, w);
				RenderWall(w_id);
			}

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 255, 100);
			int num_edges = edges.size();
			for (int e = 0; e < num_edges; e++) {
				RenderEdge(e);
			}
		}
	}

	void AfterRenderHook(void) {
		SDL_SetRenderDrawBlendMode(gameScreen.m_renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
		// RenderCollisionBitmap(IntVector(0, 0));

		IntVector playerPos = IntVector(obj.entities[0].xp, obj.entities[0].yp);
		SDL_SetRenderDrawBlendMode(gameScreen.m_renderer, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 255, 255);
		RenderPixel(playerPos.x, playerPos.y);
	}

	// -------------------
	// Rendering functions
	// -------------------

	void RenderPixel(int x, int y) {
		const SDL_Rect rect = { x, y, 1, 1 };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}

	void RenderWall(WallID w) {
		if (w.room == GetCurrentRoomPosition()) {
			RoomWall& wall = GetRoomData(w.room).walls.at(w.wallIndex);

			int x1, y1, x2, y2;
			switch (wall.type) {
				case WallType::Floor:
					y1 = y2 = wall.plane + 1;
					x1 = wall.min + (wall.minCornerConcave ? 0 : 1);
					x2 = wall.max - (wall.maxCornerConcave ? 0 : 1);
					break;
				case WallType::Ceiling:
					y1 = y2 = wall.plane - 1;
					x1 = wall.min + (wall.minCornerConcave ? 0 : 1);
					x2 = wall.max - (wall.maxCornerConcave ? 0 : 1);
					break;
				case WallType::LeftWall:
					x1 = x2 = wall.plane - 1;
					y1 = wall.min + (wall.minCornerConcave ? 0 : 1);
					y2 = wall.max - (wall.maxCornerConcave ? 0 : 1);
					break;
				case WallType::RightWall:
					x1 = x2 = wall.plane + 1;
					y1 = wall.min + (wall.minCornerConcave ? 0 : 1);
					y2 = wall.max - (wall.maxCornerConcave ? 0 : 1);
					break;
				default:
					x1 = x2 = y1 = y2 = -1;
					break;
			}

			SDL_RenderDrawLine(gameScreen.m_renderer, x1, y1, x2, y2);
		}
	}

	void RenderEdge(int edge_index) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		RoomData& currentRoomData = GetRoomData(currentRoom);
		NavigationEdge& edge = edges.at(edge_index);

		bool edgeTouchesCurrentRoom = edge.from.room == currentRoom || edge.to.room == currentRoom;
		if (!edgeTouchesCurrentRoom && !(edge.from.room == edge.to.room)) {
			// TODO: More detailed AABB check
			edgeTouchesCurrentRoom = true;
		}

		if (edgeTouchesCurrentRoom) {
			RoomData& fromRoomData = GetRoomData(edge.from.room);
			RoomData& toRoomData = GetRoomData(edge.to.room);

			NavigationNode& n1 = fromRoomData.nodes.at(edge.from.nodeIndex);
			NavigationNode& n2 = toRoomData.nodes.at(edge.to.nodeIndex);

			int d_rx = GetHOffsetBetweenRooms(currentRoom, edge.from.room);
			int d_ry = currentRoom.ry - edge.from.room.ry;

			if (d_ry < -9) {
				d_ry += 20;
			} else if (d_ry > 9) {
				d_ry -= 20;
			}

			int x1, y1, x2, y2;
			switch (n1.type) {
				default:
					return;
				case NavigationNodeType::CornerNodeType:
					{
						Corner& corner = fromRoomData.corners.at(n1.data.corner.corner.cornerIndex);
						x1 = corner.x + 320 * d_rx;
						y1 = corner.y - 240 * d_ry;
					}
					break;
				case NavigationNodeType::StartNodeType:
					{
						StartNavigationNode s = n1.data.start;
						x1 = s.pos.x + 320 * d_rx;
						y1 = s.pos.y - 240 * d_ry;
					}
					break;
			}
			switch (n2.type) {
				default:
					return;
				case NavigationNodeType::CornerNodeType:
					{
						Corner& corner = toRoomData.corners.at(n2.data.corner.corner.cornerIndex);
						x2 = corner.x;
						y2 = corner.y;
					}
					break;
				case NavigationNodeType::GoalNodeType:
					{
						GoalNavigationNode g = n2.data.goal;
						x2 = g.pos.x + 320 * d_rx;
						y2 = g.pos.y - 240 * d_ry;
					}
					break;
			}
			x2 = x1 + edge.distance.x;
			y2 = y1 + edge.distance.y;

			SDL_RenderDrawLine(gameScreen.m_renderer, x1, y1, x2, y2);
		}
	}

	void RenderCollisionBitmap(IntVector offset) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		RoomData& currentRoomData = GetRoomData(currentRoom);

		IntVector min = IntVector(currentRoomData.GetMinXPos() - 6, currentRoomData.GetMinYPos() - 10);
		IntVector max = IntVector(currentRoomData.GetMaxXPos() + 6, currentRoomData.GetMaxYPos() + 10);

		bool* collision_bitmap = GetCurrentRoomPlayerCollisionBitmap(min, max);
		int bitmap_width = max.x - min.x + 1;
		int bitmap_height = max.x - min.x + 1;

		for (int x = 0; x < bitmap_width; x++) {
			for (int y = 0; y < bitmap_height; y++) {
				if (collision_bitmap[bitmap_width * y + x]) {
					RenderPixel(x + offset.x, y + offset.y);
				}
			}
		}
		SDL_free((void*)collision_bitmap);
	}

	// --------------------------------------
	// Functions
	// --------------------------------------

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

	Corner& GetCorner(CornerID corner_id) {
		return GetRoomData(corner_id.room).corners.at(corner_id.cornerIndex);
	}

	NavigationNode& GetNavigationNode(NavigationNodeID node_id) {
		return GetRoomData(node_id.room).nodes.at(node_id.nodeIndex);
	}

	bool IsSameOrInverseNode(NavigationNodeID n1, NavigationNodeID n2) {
		if (n1.room != n2.room) {
			return false;
		}
		if (n1.nodeIndex == n2.nodeIndex) {
			return true;
		}

		NavigationNode& node1 = GetNavigationNode(n1);
		NavigationNode& node2 = GetNavigationNode(n2);
		if (node1.type != node2.type) {
			return false;
		}

		switch (node1.type) {
			default:
				return false;
			case CornerNodeType:
			{
				CornerNavigationNode c1 = node1.data.corner;
				CornerNavigationNode c2 = node2.data.corner;
				if (c1.corner == c2.corner) {
					return true;
				}
				return false;
			}
		}

		return false;
	}
	
	NavigationEdge RemoveEdge(int edgeIndex) {
		int lastIndex = edges.size() - 1;
		if (edgeIndex > lastIndex) {
			VVV_exit(1);
		} else if (edgeIndex < lastIndex) {
			// Swap with last element
			iter_swap(edges.begin() + edgeIndex, edges.begin() + lastIndex);
		}

		// Remove last
		NavigationEdge result = edges.back();
		edges.pop_back();
		return result;
	}

	void RemoveElement(std::vector<int>& v, int index) {
		int lastIndex = v.size() - 1;
		if (index > lastIndex) {
			VVV_exit(1);
		} else if (index < lastIndex) {
			// Swap with last element
			iter_swap(v.begin() + index, v.begin() + lastIndex);
		}

		v.pop_back();
	}

	void LoadRoom(RoomPosition room) {
		IntVector native_coords = room.GetNativeRoomCoords();

		if (game.roomx != native_coords.x || game.roomy != native_coords.y || map.finalmode != room.outside) {
			map.finalmode = room.outside;
			map.gotoroom(native_coords.x, native_coords.y);
		}
	}

	void InitializeConnectedRooms(RoomPosition startingRoom) {
		std::vector<RoomPosition> queue;
		queue.push_back(startingRoom);

		while (queue.size() > 0) {
			RoomPosition room = queue.back();
			queue.pop_back();

			if (GetRoomData(room).initialized) {
				// Already initialized, skip
				continue;
			}

			InitializeRoomData(room);
			RoomData& roomData = GetRoomData(room);
			if (roomData.up) {
				queue.push_back(room.NextRoomUp());
			}
			if (roomData.down) {
				queue.push_back(room.NextRoomDown());
			}
			if (roomData.left) {
				queue.push_back(room.NextRoomLeft());
			}
			if (roomData.right) {
				queue.push_back(room.NextRoomRight());
			}
		}

		LoadRoom(startingRoom);
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
			if (!collision_bitmap[(min.y - min.y + 1) * bitmap_width + (x - min.x)]) {
				result.up = true;
			}
			if (!collision_bitmap[(max.y - min.y - 1) * bitmap_width + (x - min.x)]) {
				result.down = true;
			}
		}
		for (int y = min.y + 1; y <= max.y - 1; y++) {
			if (!collision_bitmap[(y - min.y) * bitmap_width + (min.x - min.x + 1)]) {
				result.left = true;
			}
			if (!collision_bitmap[(y - min.y) * bitmap_width + (max.x - min.x - 1)]) {
				result.right = true;
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
				bool left = collision_bitmap[(y - min.y) * bitmap_width + (x - 1 - min.x)];
				bool self = collision_bitmap[(y - min.y) * bitmap_width + (x - min.x)];

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
				bool up = collision_bitmap[bitmap_width * (y - min.y - 1) + (x - min.x)];
				bool self = collision_bitmap[bitmap_width * (y - min.y) + (x - min.x)];

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
		for (int x = min.x + 1; x <= max.x - 1; x++) {
			for (int y = min.y + 1; y <= max.y - 1; y++) {
				bool up_left = collision_bitmap[bitmap_width * (y - min.y - 1) + (x - min.x - 1)];
				bool left = collision_bitmap[bitmap_width * (y - min.y) + (x - min.x - 1)];
				bool up = collision_bitmap[bitmap_width * (y - min.y - 1) + (x - min.x)];
				bool self = collision_bitmap[bitmap_width * (y - min.y) + (x - min.x)];

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

	void CreateRoomNodes(RoomPosition r) {
		RoomData& roomData = GetRoomData(r);
		if (!roomData.initialized) {
			VVV_exit(1);
			return;
		}

		if (r == start_room) {
			StartNavigationNode startNode(start_room, IntVector(start_x, start_y), start_gravity);
			roomData.nodes.emplace_back(startNode);
		}
		if (r == goal_room) {
			GoalNavigationNode goalNode(goal_room, IntVector(goal_x, goal_y));
			roomData.nodes.emplace_back(goalNode);
		}

		int num_corners = roomData.corners.size();
		int num_walls = roomData.walls.size();

		for (int c_idx = 0; c_idx < num_corners; c_idx++) {
			Corner& c = roomData.corners.at(c_idx);

			CornerID c_id(r, c_idx);

			NavigationNode n1(c_id, false);
			NavigationNode n2(c_id, true);

			roomData.nodes.push_back(n1);
			roomData.nodes.push_back(n2);
		}

		for (int w_idx = 0; w_idx < num_walls; w_idx++) {
			RoomWall& w = roomData.walls.at(w_idx);
			
			if (w.type != WallType::Floor && w.type != WallType::Ceiling) {
				continue;
			}

			WallID w_id(r, w_idx);
			// TODO: create wall nodes
		}
	}

	void ConnectNodes(NavigationNodeID from, NavigationNodeID to) {
		if (from.room.outside != to.room.outside) {
			return;
		}

		RoomData& fromRoomData = GetRoomData(from.room);
		RoomData& toRoomData = GetRoomData(to.room);
		NavigationNode n1 = fromRoomData.nodes.at(from.nodeIndex);
		NavigationNode n2 = toRoomData.nodes.at(to.nodeIndex);

		if (n1.type == NavigationNodeType::GoalNodeType || n2.type == NavigationNodeType::StartNodeType) {
			// Can't connect from goal or to start
			return;
		}
		if (n1.type == NavigationNodeType::StartNodeType && n2.type == NavigationNodeType::CornerNodeType) {
			// Start node to corner node
			StartNavigationNode s = n1.data.start;
			CornerNavigationNode c = n2.data.corner;

			Corner& cornerData = toRoomData.corners.at(c.corner.cornerIndex);

			bool gravityChange = s.inverseGravity != c.inverseGravity;
			// Gravity change implies dy == 0
			bool goingLeft = c.inverseGravity;
			bool goingUp = c.inverseGravity;
			if (cornerData.type == BottomRight || cornerData.type == TopLeft) {
				goingLeft = !c.inverseGravity;
			}

			int d_rx = GetHOffsetBetweenRooms(from.room, to.room);
			int d_x = d_rx * 320 + cornerData.x - s.pos.x;

			if (goingLeft && d_x > 0 || !goingLeft && d_x < 0) {
				return;
			}

			int d_ry_base = to.room.ry - from.room.ry;
			for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
				int d_y = d_ry * 240 + cornerData.y - s.pos.y;

				if (gravityChange && d_y != 0) {
					continue;
				}
				if (goingUp && d_y > 0 || !goingUp && d_y < 0) {
					continue;
				}

				// Check if the corner is compatible with the direction
				if (d_y == 0) {
					if (d_x == 0) {
						VVV_exit(1);
						continue;
					} else if (d_x > 0) {
						if (cornerData.type == TopLeft || cornerData.type == BottomLeft) {
							continue;
						}
					} else {
						// d_x < 0
						if (cornerData.type == TopRight || cornerData.type == BottomRight) {
							continue;
						}
					}
				} else if (d_y > 0) {
					if (d_x == 0) {
						if (cornerData.type == TopLeft || cornerData.type == TopRight) {
							continue;
						}
					} else if (d_x > 0) {
						if (cornerData.type == TopLeft || cornerData.type == BottomRight) {
							continue;
						}
					} else {
						// d_x < 0
						if (cornerData.type == TopRight || cornerData.type == BottomLeft) {
							continue;
						}
					}
				} else {
					// d_y < 0
					if (d_x == 0) {
						if (cornerData.type == BottomLeft || cornerData.type == BottomRight) {
							continue;
						}
					} else if (d_x > 0) {
						if (cornerData.type == TopRight|| cornerData.type == BottomLeft) {
							continue;
						}
					} else {
						// d_x < 0
						if (cornerData.type == TopLeft || cornerData.type == BottomRight) {
							continue;
						}
					}
				}

				Ray ray = Ray(s.pos.x, s.pos.y, d_x, d_y);
				float t = GlobalRaycast(from.room, ray);
				if (t >= 1) {
					// No intersection found between the nodes, add the edge!
					edges.emplace_back(from, to, IntVector(d_x, d_y));
					continue;
				} else if (t < 0) {
					// Something weird happened
					VVV_exit(-1);
					return;
				}
			}
			return;
		}
		if (n1.type == NavigationNodeType::CornerNodeType && n2.type == NavigationNodeType::GoalNodeType) {
			// Start node to corner node
			CornerNavigationNode c = n1.data.corner;
			GoalNavigationNode g = n2.data.goal;

			Corner& cornerData = fromRoomData.corners.at(c.corner.cornerIndex);

			// Gravity change implies dy == 0
			bool goingLeft = c.inverseGravity;
			bool goingUp = c.inverseGravity;
			if (cornerData.type == BottomRight || cornerData.type == TopLeft) {
				goingLeft = !c.inverseGravity;
			}

			int d_rx = GetHOffsetBetweenRooms(from.room, to.room);
			int d_x = d_rx * 320 + g.pos.x - cornerData.x;

			if (goingLeft && d_x > 0 || !goingLeft && d_x < 0) {
				return;
			}

			int d_ry_base = to.room.ry - from.room.ry;
			for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
				int d_y = d_ry * 240 + g.pos.y - cornerData.y;

				if (goingUp && d_y > 0 || !goingUp && d_y < 0) {
					continue;
				}

				// Check if the corner is compatible with the direction
				if (d_y == 0) {
					if (d_x == 0) {
						VVV_exit(1);
						continue;
					} else if (d_x > 0) {
						if (cornerData.type == TopRight || cornerData.type == BottomRight) {
							continue;
						}
					} else {
						// d_x < 0
						if (cornerData.type == TopLeft || cornerData.type == BottomLeft) {
							continue;
						}
					}
				} else if (d_y > 0) {
					if (d_x == 0) {
						if (cornerData.type == BottomLeft || cornerData.type == BottomRight) {
							continue;
						}
					} else if (d_x > 0) {
						if (cornerData.type == TopLeft || cornerData.type == BottomRight) {
							continue;
						}
					} else {
						// d_x < 0
						if (cornerData.type == TopRight || cornerData.type == BottomLeft) {
							continue;
						}
					}
				} else {
					// d_y < 0
					if (d_x == 0) {
						if (cornerData.type == TopLeft || cornerData.type == TopRight) {
							continue;
						}
					} else if (d_x > 0) {
						if (cornerData.type == TopRight || cornerData.type == BottomLeft) {
							continue;
						}
					} else {
						// d_x < 0
						if (cornerData.type == TopLeft || cornerData.type == BottomRight) {
							continue;
						}
					}
				}

				Ray ray = Ray(cornerData.x, cornerData.y, d_x, d_y);
				float t = GlobalRaycast(from.room, ray);
				if (t >= 1) {
					// No intersection found between the nodes, add the edge!
					edges.emplace_back(from, to, IntVector(d_x, d_y));
					continue;
				} else if (t < 0) {
					// Something weird happened
					VVV_exit(-1);
					return;
				}
			}
			return;
		}
		if (n1.type == NavigationNodeType::CornerNodeType && n2.type == NavigationNodeType::CornerNodeType) {
			// Corner to corner
			CornerNavigationNode c1 = n1.data.corner;
			CornerNavigationNode c2 = n2.data.corner;
			Corner& c1_data = fromRoomData.corners.at(c1.corner.cornerIndex);
			Corner& c2_data = toRoomData.corners.at(c2.corner.cornerIndex);

			bool gravityChange = c1.inverseGravity != c2.inverseGravity;
			// Gravity change implies dy == 0
			bool goingLeft, goingUp, goingRight, goingDown;
			goingLeft = goingUp = goingRight = goingDown = false;
			switch (c1_data.type) {
				case CornerType::BottomLeft:
				case CornerType::TopRight:
					if (c1.inverseGravity) {
						// going up left
						goingLeft = goingUp = true;
					} else {
						// going down right
						goingRight = goingDown = true;
					}
					break;
				case CornerType::BottomRight:
				case CornerType::TopLeft:
					if (c1.inverseGravity) {
						// going up right
						goingUp = goingRight = true;
					} else {
						// going down left
						goingLeft = goingDown = true;
					}
					break;
			}
			switch (c2_data.type) {
				case CornerType::BottomLeft:
				case CornerType::TopRight:
					if (c2.inverseGravity) {
						// going up left
						goingUp = goingLeft = true;
					} else {
						// going down right
						goingDown = goingRight = true;
					}
					break;
				case CornerType::BottomRight:
				case CornerType::TopLeft:
					if (c2.inverseGravity) {
						// going up right
						goingUp = goingRight = true;
					} else {
						// going down left
						goingDown = goingLeft = true;
					}
					break;
			}

			int d_rx = GetHOffsetBetweenRooms(from.room, to.room);
			int d_x = d_rx * 320 + c2_data.x - c1_data.x;

			if (goingLeft && d_x > 0 || goingRight && d_x < 0) {
				return;
			}

			int d_ry_base = to.room.ry - from.room.ry;
			for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
				int d_y = d_ry * 240 + c2_data.y - c1_data.y;

				if (gravityChange && d_y != 0) {
					continue;
				}
				if (goingUp && d_y > 0 || !goingUp && d_y < 0) {
					continue;
				}

				// Check if the corners are compatible
				if (!CanConnectCorners(c1_data.type, c2_data.type, IntVector(d_x, d_y))) {
					continue;
				}

				Ray ray = Ray(c1_data.x, c1_data.y, d_x, d_y);
				float t = GlobalRaycast(from.room, ray);
				if (t >= 1) {
					// No intersection found between the corners, add the edge!
					edges.emplace_back(from, to, IntVector(d_x, d_y));
					continue;
				} else if (t < 0) {
					// Something weird happened
					VVV_exit(-1);
					return;
				}
			}
			return;
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

	bool CanConnectEdges(NavigationEdge& e1, NavigationEdge& e2) {
		if (e1.from == e2.to || e1.to != e2.from) {
			// Has to connect through same node, can't go back to original
			return false;
		}
		if (e1.distance.x == -e2.distance.x && e1.distance.y == -e2.distance.y) {
			// Can't be exact inverse
			return false;
		}

		NavigationNode& firstNode = GetNavigationNode(e1.from);
		NavigationNode& middleNode = GetNavigationNode(e1.to);
		NavigationNode& lastNode = GetNavigationNode(e2.to);

		IntVector d1 = e1.distance;
		IntVector d2 = e2.distance;

		// We mostly care about the type of the middle node
		switch (middleNode.type) {
			default:
				return false;
			case CornerNodeType:
			{
				// Can't change axis direction without going through 0
				if (d1.x < 0 && d2.x > 0 || d1.x > 0 && d2.x < 0) {
					return false;
				}
				if (d1.y < 0 && d2.y > 0 || d1.y > 0 && d2.y < 0) {
					return false;
				}

				bool inverseGravity = middleNode.data.corner.inverseGravity;
				Corner& corner = GetCorner(middleNode.data.corner.corner);
				bool slopeIncrease;
				switch (corner.type) {
					default:
						return false;
					case TopLeft:
						if (inverseGravity) {
							if (d1.x == 0 && d2.y == 0) {
								return true;
							}
							slopeIncrease = false;
						} else {
							if (d1.y == 0 && d2.x == 0) {
								return true;
							}
							slopeIncrease = true;
						}
						break;
					case TopRight:
						if (inverseGravity) {
							if (d1.x == 0 && d2.y == 0) {
								return true;
							}
							slopeIncrease = true;
						} else {
							if (d1.y == 0 && d2.x == 0) {
								return true;
							}
							slopeIncrease = false;
						}
						break;
					case BottomRight:
						if (inverseGravity) {
							if (d1.y == 0 && d2.x == 0) {
								return true;
							}
							slopeIncrease = true;
						} else {
							if (d1.x == 0 && d2.y == 0) {
								return true;
							}
							slopeIncrease = false;
						}
						break;
					case BottomLeft:
						if (inverseGravity) {
							if (d1.y == 0 && d2.x == 0) {
								return true;
							}
							slopeIncrease = false;
						} else {
							if (d1.x == 0 && d2.y == 0) {
								return true;
							}
							slopeIncrease = true;
						}
						break;
				}

				// Angle must be greater than 180 degrees (bend around corner)
				// -> slope of edge should increase / decrease depending on corner type
				// Note that Y axis is inverted so slopes are backwards
				// Slope increase: d1.y / d1.x < d2.y / d2.x
				// Slope decrease: d1.y / d1.x > d2.y / d2.x
				// Since the xs have the same sign (or are 0) we can multiply by both without changing the direction of the inequality
				// Slope increase: d1.y * d2.x < d2.y * d1.x
				// Slope decrease: d1.y * d2.x > d2.y * d1.x
				// Since the Y axes are inverted, the conditions are backwards
				int slopeBefore = d1.y * d2.x;
				int slopeAfter = d2.y * d1.x;
				if (slopeBefore < slopeAfter) {
					return !slopeIncrease;
				} else if (slopeBefore > slopeAfter) {
					return slopeIncrease;
				} else {
					// Slopes are the same
					return false;
				}
				break;
			}
		}

		return false;
	}

	int PruneDeadEndEdges(void) {
		int totalRemoved = 0;
		// Prune dead end edges
		int e = 0;
		int numRemoved = 0;
		while (e < edges.size() || numRemoved > 0) {
			if (e >= edges.size()) {
				e = 0;
				numRemoved = 0;
			}

			NavigationEdge& edge = edges.at(e);

			NavigationNode& fromNode = GetRoomData(edge.from.room).nodes.at(edge.from.nodeIndex);
			NavigationNode& toNode = GetRoomData(edge.to.room).nodes.at(edge.to.nodeIndex);

			bool connectIn = fromNode.type == StartNodeType;
			bool connectOut = toNode.type == GoalNodeType;
			for (int e2 = 0; e2 < edges.size() && (!connectIn || !connectOut); e2++) {
				NavigationEdge& other = edges.at(e2);
				if (e == e2) {
					continue;
				}
				if (edge.to != other.from && edge.from != other.to) {
					continue;
				}
				if (!connectOut && CanConnectEdges(edge, other)) {
					connectOut = true;
				}
				if (!connectIn && CanConnectEdges(other, edge)) {
					connectIn = true;
				}
			}

			if (connectIn && connectOut) {
				e++;
			} else {
				RemoveEdge(e);
				numRemoved += 1;
				totalRemoved += 1;
			}
		}

		return totalRemoved;
	}

	// TODO: domination doesn't respect how edges connect
	int PruneDominatedEdges(void) {
		// -> Find places where a node dominates a sub-graph both from the perspective of the start node and the goal node

		// Algorithm:
		// dominator of the start node is the start itself
		//     Dom(n0) = { n0 }
		// for all other nodes, set all nodes as the dominators
		//     for each n in N - {n0}
		//         Dom(n) = N;
		// iteratively eliminate nodes that are not dominators
		//     while changes in any Dom(n)
		//         for each n in N - {n0}:
		//             Dom(n) = { n } union with intersection over Dom(p) for all p in pred(n)

		// Need to keep track of:
		//   1. dominator set for all nodes, starting from start node
		//   2. dominator set for all nodes, starting from goal node
		// if the intersection of these two sets for a node is not empty, remove its edges from the graph
		std::vector<NavigationNodeID> queue;
		for (int rx = 0; rx < 20; rx++) {
			for (int ry = 0; ry < 20; ry++) {
				RoomPosition room(GetCurrentRoomPosition().outside, rx, ry);
				RoomData& roomData = GetRoomData(room);
				for (int n = 0; n < roomData.nodes.size(); n++) {
					NavigationNode& node = roomData.nodes.at(n);
					if (node.type == StartNodeType || node.type == GoalNodeType) {
						queue.emplace_back(room, n);
					}
				}
			}
		}

		std::vector<NavigationNodeID> reachable_nodes;
		while (queue.size() > 0) {
			NavigationNodeID node_id = queue.back();
			queue.pop_back();

			bool alreadyReached = false;
			for (int i = 0; i < reachable_nodes.size(); i++) {
				if (reachable_nodes.at(i) == node_id) {
					alreadyReached = true;
					break;
				}
			}
			if (alreadyReached) {
				continue;
			}

			reachable_nodes.push_back(node_id);

			for (int e = 0; e < edges.size(); e++) {
				NavigationEdge& edge = edges.at(e);
				if (edge.to == node_id) {
					queue.push_back(edge.from);
				}
				if (edge.from == node_id) {
					queue.push_back(edge.to);
				}
			}
		}

		std::vector<NavigationNodeID> reachable_node_duplicates;
		// De-duplicate reachable nodes
		for (int n = 0; n < reachable_nodes.size(); ) {
			bool is_duplicate = false;
			for (int n2 = 0; n2 < n; n2++) {
				if (IsSameOrInverseNode(reachable_nodes.at(n), reachable_nodes.at(n2))) {
					is_duplicate = true;
					break;
				}
			}

			if (is_duplicate) {
				int lastIndex = reachable_nodes.size() - 1;
				if (n < lastIndex) {
					// Swap with last element
					iter_swap(reachable_nodes.begin() + n, reachable_nodes.begin() + lastIndex);
				}
				reachable_node_duplicates.push_back(reachable_nodes.back());
				reachable_nodes.pop_back();
			}
			else {
				n++;
			}
		}

		const int num_reachable_nodes = reachable_nodes.size();
		std::vector<std::vector<int>> predecessor_list;
		std::vector<std::vector<int>> successor_list;
		std::vector<std::set<int>> start_dominator_sets;
		std::vector<std::set<int>> goal_dominator_sets;
		int start_node = num_reachable_nodes;
		int goal_node = num_reachable_nodes;
		for (int n = 0; n < num_reachable_nodes; n++) {
			// Add empty lists
			predecessor_list.emplace_back();
			successor_list.emplace_back();
			start_dominator_sets.emplace_back();
			goal_dominator_sets.emplace_back();

			// Remember start and goal nodes
			// Set up dominator lists
			NavigationNode& node = GetNavigationNode(reachable_nodes.at(n));
			if (node.type == StartNodeType) {
				start_node = n;
				// Only start node dominates itself
				start_dominator_sets.back().insert(n);
			}
			else {
				// Add all nodes to dominator list
				for (int n2 = 0; n2 < num_reachable_nodes; n2++) {
					start_dominator_sets.back().insert(n2);
				}
			}

			if (node.type == GoalNodeType) {
				goal_node = n;
				// Only goal node dominates itself
				goal_dominator_sets.back().insert(n);
			}
			else {
				// Add all nodes to dominator list
				for (int n2 = 0; n2 < num_reachable_nodes; n2++) {
					goal_dominator_sets.back().insert(n2);
				}
			}
		}

		// Add edges to adjacency list
		for (int e = 0; e < edges.size(); e++) {
			NavigationEdge& edge = edges.at(e);
			int fromIndex = num_reachable_nodes;
			for (int n = 0; n < num_reachable_nodes; n++) {
				if (IsSameOrInverseNode(reachable_nodes.at(n), edge.from)) {
					fromIndex = n;
					break;
				}
			}
			int toIndex = num_reachable_nodes;
			for (int n = 0; n < num_reachable_nodes; n++) {
				if (IsSameOrInverseNode(reachable_nodes.at(n), edge.to)) {
					toIndex = n;
					break;
				}
			}
			if (fromIndex < num_reachable_nodes && toIndex < num_reachable_nodes) {
				// Edge is between reachable nodes, add it to adjacency list
				predecessor_list.at(toIndex).push_back(fromIndex);
				successor_list.at(fromIndex).push_back(toIndex);
			}
		}

		// Run dataflow algorithm
		bool anythingChanged = true;
		while (anythingChanged) {
			anythingChanged = false;

			for (int n = 0; n < num_reachable_nodes; n++) {
				std::set<int> new_start_dominator_set;
				std::set<int> new_goal_dominator_set;
				for (int n2 = 0; n2 < num_reachable_nodes; n2++) {
					if (n != start_node) {
						new_start_dominator_set.insert(n2);
					}
					if (n != goal_node) {
						new_goal_dominator_set.insert(n2);
					}
				}
				// Intersection with all predecessors
				int num_predecessors = predecessor_list.at(n).size();
				for (int p = 0; p < num_predecessors; p++) {
					int predecessor = predecessor_list.at(n).at(p);
					std::set<int> results;
					std::set_intersection(new_start_dominator_set.begin(), new_start_dominator_set.end(), start_dominator_sets.at(predecessor).begin(), start_dominator_sets.at(predecessor).end(), std::inserter(results, results.begin()));
					new_start_dominator_set.clear();
					new_start_dominator_set.insert(results.begin(), results.end());
				}
				// Intersection with all successors
				int num_successors = successor_list.at(n).size();
				for (int s = 0; s < num_successors; s++) {
					int successor = successor_list.at(n).at(s);
					std::set<int> results;
					std::set_intersection(new_goal_dominator_set.begin(), new_goal_dominator_set.end(), goal_dominator_sets.at(successor).begin(), goal_dominator_sets.at(successor).end(), std::inserter(results, results.begin()));
					new_goal_dominator_set.clear();
					new_goal_dominator_set.insert(results.begin(), results.end());
				}

				// Every node dominates itself
				new_start_dominator_set.insert(n);
				new_goal_dominator_set.insert(n);

				// Check if elements changed
				bool shouldCopy = false;
				if (new_start_dominator_set.size() != start_dominator_sets.at(n).size()) {
					shouldCopy = true;
				}
				else if (new_goal_dominator_set.size() != goal_dominator_sets.at(n).size()) {
					shouldCopy = true;
				}
				if (!shouldCopy) {
					for (std::set<int>::iterator it = new_start_dominator_set.begin(); it != new_start_dominator_set.end(); ++it) {
						int predecessor = *it;
						if (start_dominator_sets.at(n).count(predecessor) == 0) {
							// This element is new
							shouldCopy = true;
						}
					}
					for (std::set<int>::iterator it = new_goal_dominator_set.begin(); it != new_goal_dominator_set.end(); ++it) {
						int successor = *it;
						if (goal_dominator_sets.at(n).count(successor) == 0) {
							// This element is new
							shouldCopy = true;
						}
					}
				}

				if (shouldCopy) {
					// Copy new dominator sets to set list
					start_dominator_sets.at(n).clear();
					start_dominator_sets.at(n).insert(new_start_dominator_set.begin(), new_start_dominator_set.end());
					goal_dominator_sets.at(n).clear();
					goal_dominator_sets.at(n).insert(new_goal_dominator_set.begin(), new_goal_dominator_set.end());
					anythingChanged = true;
				}
			}
		}

		auto cmp = [](NavigationNodeID a, NavigationNodeID b) {
			if (a.room.outside < b.room.outside) {
				return true;
			}
			else if (a.room.outside == b.room.outside) {
				if (a.room.rx < b.room.rx) {
					return true;
				}
				else if (a.room.rx == b.room.rx) {
					if (a.room.ry < b.room.ry) {
						return true;
					}
					else if (a.room.ry == b.room.ry) {
						return a.nodeIndex < b.nodeIndex;
					}
				}
			}
			return false;
			};
		std::set<NavigationNodeID, std::function<bool(NavigationNodeID, NavigationNodeID)>> double_dominated(cmp);
		std::set<NavigationNodeID, std::function<bool(NavigationNodeID, NavigationNodeID)>> start_dominators(cmp);
		std::set<NavigationNodeID, std::function<bool(NavigationNodeID, NavigationNodeID)>> goal_dominators(cmp);

		for (int n = 0; n < num_reachable_nodes; n++) {
			NavigationNodeID node_id = reachable_nodes.at(n);

			std::set<int> results;
			std::set_intersection(start_dominator_sets.at(n).begin(), start_dominator_sets.at(n).end(), goal_dominator_sets.at(n).begin(), goal_dominator_sets.at(n).end(), std::inserter(results, results.begin()));
			if (results.size() > 1 && n != start_node && n != goal_node) {
				// A node should always be dominated by itself
				// Any more means it's a "dead end"
				double_dominated.insert(node_id);
			}
			// Remember which nodes dominate start and goal nodes (they are bottlenecks)
			if (start_dominator_sets.at(goal_node).count(n) > 0) {
				goal_dominators.insert(node_id);
			}
			if (goal_dominator_sets.at(start_node).count(n) > 0) {
				start_dominators.insert(node_id);
			}
		}

		// Find the nodes we want to keep
		std::set<NavigationNodeID, std::function<bool(NavigationNodeID, NavigationNodeID)>> nodes_to_keep(cmp);
		for (int n = 0; n < reachable_nodes.size(); n++) {
			NavigationNodeID node = reachable_nodes.at(n);

			if (double_dominated.count(node) == 0) {
				nodes_to_keep.insert(node);
			}
		}
		// Re-duplicate nodes
		for (int n = 0; n < reachable_node_duplicates.size(); n++) {
			NavigationNodeID duplicate_node = reachable_node_duplicates.at(n);

			bool shouldInsert = false;
			for (std::set<NavigationNodeID, std::function<bool(NavigationNodeID, NavigationNodeID)>>::iterator it = nodes_to_keep.begin(); it != nodes_to_keep.end(); ++it) {
				NavigationNodeID node_to_keep = *it;

				if (IsSameOrInverseNode(duplicate_node, node_to_keep)) {
					shouldInsert = true;
					break;
				}
			}

			if (shouldInsert) {
				nodes_to_keep.insert(duplicate_node);
			}
		}

		// Prune edges that don't connect to keepable nodes
		int numRemoved = 0;
		for (int e = 0; e < edges.size();) {
			NavigationEdge& edge = edges.at(e);
			if (nodes_to_keep.count(edge.from) == 0 || nodes_to_keep.count(edge.to) == 0) {
				// Edge can be removed
				RemoveEdge(e);
				numRemoved++;
			}
			else {
				e++;
			}
		}

		return numRemoved;
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
				bitmap[(y - min.y) * x_extent + (x - min.x)] = collision;
			}
		}

		return bitmap;
	}

	// ------------------------
	// Raycasting Functionality
	// ------------------------
	float GlobalRaycast(RoomPosition startingRoom, Ray& startingRay) {
		RoomPosition currentRoom = startingRoom;
		Ray ray = startingRay;

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

			bool rayEntersRoom = t_min < t_max;
			bool rayStartsInRoom = rayEntersRoom && t_min <= 0;
			bool rayEndsInRoom = rayEntersRoom && t_max > 1;

			// Check if the side the ray enters from is traversable
			if (rayEntersRoom && !rayStartsInRoom) {
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

			if (rayEntersRoom) {
				float room_result = RoomRaycast(currentRoom, ray);
				if (0 <= room_result && room_result < INFINITY) {
					return room_result;
				}
			}

			// Failsafe: if the exiting side is not traversable, stop the ray at the exiting side
			// TODO: Could happen with warping rooms for example, not sure if this is the right approach
			if (!rayEntersRoom || !rayEndsInRoom) {
				if (t_top == t_max) {
					// Ray exits top edge of screen
					if (room_data.up) {
						currentRoom = currentRoom.NextRoomUp();
						ray.origin.y += 240;
					} else {
						return t_max;
					}
				}
				if (t_bottom == t_max) {
					if (room_data.down) {
						// Ray exits bottom edge of screen
						currentRoom = currentRoom.NextRoomDown();
						ray.origin.y -= 240;
					} else {
						return t_max;
					}
				}
				if (t_left == t_max) {
					if (room_data.left) {
						// Ray exits left edge of screen
						currentRoom = currentRoom.NextRoomLeft();
						ray.origin.x += 320;
					} else {
						return t_max;
					}
				}
				if (t_right == t_max) {
					if (room_data.right) {
						// Ray exits right edge of screen
						currentRoom = currentRoom.NextRoomRight();
						ray.origin.x -= 320;
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
				int x2 = ray.direction.x;
				int y1 = wall.plane - ray.origin.y;
				int y2 = ray.direction.y;
				minCmp = x1 * y2 - x2 * y1;
				// Does the ray exactly intersect the maxCorner?
				x1 = wall.max - ray.origin.x;
				x2 = ray.direction.x;
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
				int x1 = wall.plane - ray.origin.x; // 84
				int x2 = ray.direction.x;			// 132
				int y1 = wall.min - ray.origin.y;	// -12
				int y2 = ray.direction.y;			// 0
				minCmp = x1 * y2 - x2 * y1;			// 
				// Does the ray exactly intersect the maxCorner?
				y1 = wall.max - ray.origin.y;		// 37
				y2 = ray.direction.y;				// 0
				maxCmp = x1 * y2 - x2 * y1;			// 
			}
		}

		if (t < 0 || t >= 1) {
			return INFINITY;
		} else if (t == 0) {
			switch (wall.type) {
				case WallType::Floor:
					if (ray.direction.y <= 0) {
						return INFINITY;
					}
					break;
				case WallType::Ceiling:
					if (ray.direction.y >= 0) {
						return INFINITY;
					}
					break;
				case WallType::LeftWall:
					if (ray.direction.x >= 0) {
						return INFINITY;
					}
					break;
				case WallType::RightWall:
					if (ray.direction.x <= 0) {
						return INFINITY;
					}
					break;
			}
		}

		if (minCmp > 0 && maxCmp < 0 || minCmp < 0 && maxCmp > 0) {
			// Ray intersects between minCorner and maxCorner -> intersection!
			return t;
		} else if (maxCmp == 0) {
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
		}

		// Failsafe: no intersection
		return INFINITY;
	}
}
