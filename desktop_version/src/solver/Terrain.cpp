#include "solver/Terrain.h"

namespace Terrain {
	// Lab IL start
	// RoomPosition start_room = RoomPosition(2, 16);
	// int start_x = 191;
	// int start_y = 33;
	// bool start_gravity = 0;

	// Letter G
	RoomPosition start_room = RoomPosition(3, 16);
	int start_x = 40;
	int start_y = 180;
	bool start_gravity = 1;
	NavigationNodeID start_node(start_room, 0);

	// Lab IL end
	// RoomPosition goal_room = RoomPosition(4, 4);
	// int goal_x = 10;
	// int goal_y = 170;
	
	// Rascasse top left corner
	// RoomPosition goal_room = RoomPosition(1, 17);
	// int goal_x = 10;
	// int goal_y = 46;

	// Barani Barani
	RoomPosition goal_room = RoomPosition(6, 16);
	int goal_x = 160;
	int goal_y = 230;
	NavigationNodeID goal_node(goal_room, 0);

	RoomData overworldRoomData[20][20];
	RoomData outsideRoomData[20][20];
	CollisionSetting collisionSetting = CollisionSetting::WallsAndSpikes;

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

			return;
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
				// numPruned += PruneDominatedEdges();
				numPruned += PruneBackAndCrossedEdges();
				if (numPruned == 0) {
					break;
				} else {
					totalPruned += numPruned;
				}
			}

			LoadRoom(RoomPosition(3, 16));
		}
	}

	int counter = 0;

	void AfterTileRenderHook(void) {
		BeforeRenderHook();

		RoomPosition currentRoom = GetCurrentRoomPosition();
		RoomData& currentRoomData = GetRoomData(currentRoom);

		GlobalPosition playerPos = GetPlayerPosition();

		if (currentRoomData.initialized) {
			SDL_SetRenderDrawBlendMode(gameScreen.m_renderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);

			if (currentRoomData.corners.empty()) {
				return;
			}

			std::vector<WaypointPath> all_paths;
			/*
			for (int c = 0; c < currentRoomData.corners.size(); c++) {
				CornerID c_id(currentRoom, c, false);
				std::vector<WaypointPath> paths = NewFindCornerConnections(c_id);
				all_paths.insert(all_paths.end(), paths.begin(), paths.end());
				CornerID c_id2(currentRoom, c, true);
				std::vector<WaypointPath> paths2 = NewFindCornerConnections(c_id2);
				all_paths.insert(all_paths.end(), paths2.begin(), paths2.end());
			}
			*/
			{
				CornerID c_id(currentRoom, 0, false);
				std::vector<WaypointPath> paths = RevampedFindCornerConnections(c_id);
				all_paths.insert(all_paths.end(), paths.begin(), paths.end());
			}
			game.timetrialshinytarget = all_paths.size();

			if (all_paths.size() != 0) {
				game.totalflips %= all_paths.size();
			}
			RenderWaypointPath(all_paths.at(game.totalflips));
			return;

			int num_walls = currentRoomData.walls.size();
			for (int w = 0; w < num_walls; w++) {
				WallID w_id(currentRoom, w);
				RoomWall& wall = GetWall(w_id);
				RenderWall(w_id);
			}

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
			int num_corners = currentRoomData.corners.size();
			for (int c = 0; c < num_corners; c++) {
				CornerID c_id(currentRoom, c, false);
				RenderCorner(c_id);
			}

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 255, 100);
			int num_edges = edges.size();
			for (int e = 0; e < num_edges; e++) {
				NavigationEdge& edge = edges.at(e);
				if (IsEdgePossibleWithoutFlipping(edge)) {
					RenderEdge(e);
				}
			}

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 0, 100);
			for (int e = 0; e < num_edges; e++) {
				NavigationEdge& edge = edges.at(e);
				if (!IsEdgePossibleWithoutFlipping(edge)) {
					RenderEdge(e);
				}
			}

			int num_lines = currentRoomData.lines.size();
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 0, 255, 100);
			for (int l = 0; l < num_lines; l++) {
				LineID line_id(currentRoom, l);
				// RenderGravityLine(line_id);
			}

			if (num_edges > 0) {
				// BuildSurfaceConnectionGraph(edges.at(0).from, edges.at(0).to, edges.at(0).distance.y < 0);
				VisualizeHeuristic();
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
		const SDL_Rect rect = { x + VIRIDIAN_CX, y + VIRIDIAN_CY, 1, 1 };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}
	void RenderPixel(const IntVector& pos) {
		const SDL_Rect rect = { pos.x + VIRIDIAN_CX, pos.y + VIRIDIAN_CY, 1, 1 };
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

			SDL_RenderDrawLine(gameScreen.m_renderer, x1 + VIRIDIAN_CX, y1 + VIRIDIAN_CY, x2 + VIRIDIAN_CX, y2 + VIRIDIAN_CY);
		}
	}

	void RenderCorner(CornerID corner_id) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		if (corner_id.room != currentRoom) {
			return;
		}

		Corner& corner = GetCorner(corner_id);
		if (corner.simple) {
			int signedVerticalGap = (corner.type == CornerType::TopLeft || corner.type == CornerType::TopRight) ? -corner.verticalGap : corner.verticalGap;
			int signedHorizontalGap = (corner.type == CornerType::TopLeft || corner.type == CornerType::BottomLeft) ? -corner.horizontalGap : corner.horizontalGap;

			GlobalPosition cornerPos(currentRoom, corner.pos);
			GlobalPosition regionPos(currentRoom, corner.pos + IntVector(signedHorizontalGap, signedVerticalGap));

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 50);
			RenderRect(cornerPos, regionPos);
		} else {
			return;
		}

		if (corner.horizontalNegativeGap != -1) {
			int signedVerticalGap = (corner.type == CornerType::TopLeft || corner.type == CornerType::TopRight) ? -corner.verticalGap : corner.verticalGap;
			int signedHorizontalGap = (corner.type == CornerType::TopLeft || corner.type == CornerType::BottomLeft) ? corner.horizontalNegativeGap : -corner.horizontalNegativeGap;

			GlobalPosition cornerPos(currentRoom, corner.pos);
			GlobalPosition regionPos(currentRoom, corner.pos + IntVector(signedHorizontalGap, signedVerticalGap));

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 0, 255, 50);
			RenderRect(cornerPos, regionPos);
		}

		if (corner.verticalNegativeGap != -1) {
			int signedVerticalGap = (corner.type == CornerType::TopLeft || corner.type == CornerType::TopRight) ? corner.verticalNegativeGap : -corner.verticalNegativeGap;
			int signedHorizontalGap = (corner.type == CornerType::TopLeft || corner.type == CornerType::BottomLeft) ? -corner.horizontalGap : corner.horizontalGap;

			GlobalPosition cornerPos(currentRoom, corner.pos);
			GlobalPosition regionPos(currentRoom, corner.pos + IntVector(signedHorizontalGap, signedVerticalGap));

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 0, 255, 50);
			RenderRect(cornerPos, regionPos);
		}
	}

	void RenderLine(const IntVector& from, const IntVector& to) {
		SDL_RenderDrawLine(gameScreen.m_renderer, from.x + VIRIDIAN_CX, from.y + VIRIDIAN_CY, to.x + VIRIDIAN_CX, to.y + VIRIDIAN_CY);
	}
	void RenderRegion(const Region& region) {
		if (region.is_bottom()) {
			return;
		}

		Region screenBounds(IntInterval(0, 319) - VIRIDIAN_CX, IntInterval(0, 239) - VIRIDIAN_CY);
		screenBounds.intersect(region);
		if (screenBounds.is_bottom() || !screenBounds.is_bounded()) {
			return;
		}

		const SDL_Rect rect = { screenBounds.x.min + VIRIDIAN_CX, screenBounds.y.min + VIRIDIAN_CY, screenBounds.x.max - screenBounds.x.min + 1, screenBounds.y.max - screenBounds.y.min + 1 };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
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
						x1 = corner.pos.x + 320 * d_rx;
						y1 = corner.pos.y - 240 * d_ry;
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
						x2 = corner.pos.x;
						y2 = corner.pos.y;
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

			SDL_RenderDrawLine(gameScreen.m_renderer, x1 + VIRIDIAN_CX, y1 + VIRIDIAN_CY, x2 + VIRIDIAN_CX, y2 + VIRIDIAN_CY);
		}
	}

	void RenderGravityLine(LineID line_id) {
		RoomPosition currentRoom = GetCurrentRoomPosition();

		if (currentRoom != line_id.room) {
			return;
		}

		GravityLine& gravityLine = GetGravityLine(line_id);
		IntVector dims(gravityLine.max.x - gravityLine.min.x + 1, gravityLine.max.y - gravityLine.min.y + 1);
		const SDL_Rect rect = { gravityLine.min.x + VIRIDIAN_CX, gravityLine.min.y + VIRIDIAN_CY, dims.x, dims.y };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}

	void RenderRect(GlobalPosition from, GlobalPosition to) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		GlobalPosition origin(currentRoom, IntVector(0, 0));

		IntVector diagonal = GetMinDistanceBetween(from, to);
		IntVector fromOffset = GetMinDistanceBetween(origin, from);
		IntVector toOffset = GetMinDistanceBetween(origin, to);

		if (toOffset.length_squared() > fromOffset.length_squared()) {
			toOffset = fromOffset + diagonal;
		} else {
			fromOffset = toOffset - diagonal;
		}

		IntVector minPos(SDL_min(fromOffset.x, toOffset.x), SDL_min(fromOffset.y, toOffset.y));
		IntVector maxPos(SDL_max(fromOffset.x, toOffset.x), SDL_max(fromOffset.y, toOffset.y));
		IntVector dims(maxPos.x - minPos.x + 1, maxPos.y - minPos.y + 1);
		const SDL_Rect rect = { minPos.x + VIRIDIAN_CX, minPos.y + VIRIDIAN_CY, dims.x, dims.y };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}

	void RenderCollisionBitmap(IntVector offset) {
		RoomPosition currentRoom = GetCurrentRoomPosition();
		RoomData& currentRoomData = GetRoomData(currentRoom);

		IntVector min = IntVector(currentRoomData.GetMinXPos() - 6, currentRoomData.GetMinYPos() - 10);
		IntVector max = IntVector(currentRoomData.GetMaxXPos() + 6, currentRoomData.GetMaxYPos() + 10);

		uint8_t* collision_bitmap = GetCurrentRoomPlayerCollisionBitmap(min, max);
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

	void RenderFullConnection(const FullConnection& conn) {
		const Corner& fromCorner = GetCorner(conn.fromCorner.corner);
		const Corner& toCorner = GetCorner(conn.toCorner.corner);

		// From region (blue)
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 0, 255, 150);
		RenderRegion(conn.fromCorner.pos);

		// To corner + region (red)
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 0, 0, 150);
		RenderRegion(conn.toCorner.pos);

		// Intermediate surfaces (yellow)
		// Also connection lines (green)
		IntVector lastPos = (conn.fromCorner.pos.getMin() + conn.fromCorner.pos.getMax()) / 2;
		IntVector thisPos;

		for (std::vector<SurfaceConnection>::const_iterator it = conn.intermediate_surfaces.begin(); it != conn.intermediate_surfaces.end(); it++) {
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 0, 150);
			RenderRegion((*it).pos);
			thisPos = ((*it).pos.getMin() + (*it).pos.getMax()) / 2;
			if ((*it).inverseGravity) {
				thisPos.y += 1;
			} else {
				thisPos.y -= 1;
			}
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
			RenderLine(lastPos, thisPos);
			lastPos = thisPos;
		}
		thisPos = (conn.toCorner.pos.getMin() + conn.toCorner.pos.getMax()) / 2;
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
		RenderLine(lastPos, thisPos);
		lastPos = thisPos;

		// Corners (blue / red / white)
		if (conn.fromCorner.corner == conn.toCorner.corner) {
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 255, 255);
		} else {
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 100, 100, 255);
			if (conn.toCorner.corner.room == GetCurrentRoomPosition()) {
				RenderPixel(toCorner.pos);
				RenderPixel(toCorner.pos - toCorner.GetPrimaryDir(conn.toCorner.goingUp));
				RenderPixel(toCorner.pos + toCorner.GetSecondaryDir(conn.toCorner.goingUp));
			}
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 100, 100, 255, 255);
		}
		if (conn.fromCorner.corner.room == GetCurrentRoomPosition()) {
			RenderPixel(fromCorner.pos);
			RenderPixel(fromCorner.pos - fromCorner.GetPrimaryDir(conn.fromCorner.goingUp));
			RenderPixel(fromCorner.pos + fromCorner.GetSecondaryDir(conn.fromCorner.goingUp));
		}
	}

	void RenderWaypointPath(const WaypointPath& path) {
		const Corner& fromCorner = GetCorner(path.source.corner_id);
		const Corner& toCorner = GetCorner(path.target.corner_id);

		IntVector roomOffset = GetMinDistanceOffsetBetweenRooms(GetCurrentRoomPosition(), path.source.corner_id.room);

		// From region (blue)
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 0, 255, 150);
		Region fromRegion(path.source.playerState.pos);
		fromRegion.x += roomOffset.x;
		fromRegion.y += roomOffset.y;
		RenderRegion(fromRegion);

		// To corner + region (red)
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 0, 0, 150);
		Region toRegion(path.target.playerState.pos);
		toRegion.x += roomOffset.x;
		toRegion.y += roomOffset.y;
		RenderRegion(toRegion);

		// Intermediate surfaces (yellow)
		// Also connection lines (green)
		IntVector lastPos = (fromRegion.getMin() + fromRegion.getMax()) / 2;
		IntVector thisPos;

		for (std::vector<GenericWaypoint>::const_iterator it = path.waypoints.cbegin(); it != path.waypoints.cend(); it++) {
			const GenericWaypoint& wp = *it;
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 0, 150);
			Region waypointRegion(wp.playerState.pos);
			waypointRegion.x += roomOffset.x;
			waypointRegion.y += roomOffset.y;
			waypointRegion.x.addLowerBound(waypointRegion.x.getLowerBound() + 1);
			waypointRegion.x.addUpperBound(waypointRegion.x.getUpperBound() - 1);
			RenderRegion(waypointRegion);
			thisPos = (waypointRegion.getMin() + waypointRegion.getMax()) / 2;
			if (wp.id.isWall()) {
				if (GetWall(wp.id.unwrapWall()).type == Floor) {
					thisPos.y -= 1;
				} else if (GetWall(wp.id.unwrapWall()).type == Ceiling) {
					thisPos.y += 1;
				}
			}
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
			RenderLine(lastPos, thisPos);
			lastPos = thisPos;
		}
		thisPos = (toRegion.getMin() + toRegion.getMax()) / 2;
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 150);
		RenderLine(lastPos, thisPos);
		lastPos = thisPos;

		// Corners (blue / red / white)
		if (path.source.corner_id.is_same_corner(path.target.corner_id)) {
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 255, 255);
		} else {
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 100, 100, 255);
			if (path.target.corner_id.room == GetCurrentRoomPosition()) {
				IntVector pDir = toCorner.GetPrimaryDir(path.target.corner_id.goingUp);
				IntVector sDir = toCorner.GetSecondaryDir(path.target.corner_id.goingUp);
				RenderPixel(toCorner.pos - pDir + sDir);
				RenderPixel(toCorner.pos - pDir - pDir + sDir);
				RenderPixel(toCorner.pos + sDir - pDir + sDir);
				RenderPixel(toCorner.pos - pDir - pDir - pDir + sDir);
				RenderPixel(toCorner.pos - pDir + sDir + sDir + sDir);
			}
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 100, 100, 255, 255);
		}
		if (path.source.corner_id.room == GetCurrentRoomPosition()) {
			IntVector pDir = fromCorner.GetPrimaryDir(path.source.corner_id.goingUp);
			IntVector sDir = fromCorner.GetSecondaryDir(path.source.corner_id.goingUp);
			RenderPixel(fromCorner.pos - pDir + sDir);
			RenderPixel(fromCorner.pos - pDir - pDir + sDir);
			RenderPixel(fromCorner.pos + sDir - pDir + sDir);
			RenderPixel(fromCorner.pos - pDir - pDir - pDir + sDir);
			RenderPixel(fromCorner.pos - pDir + sDir + sDir + sDir);
		}
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
	Corner GetCornerInLocalFrame(CornerID corner_id, const LocalFrame& frame) {
		RoomData& roomData = GetRoomData(corner_id.room);
		Corner& rawCorner = roomData.corners.at(corner_id.cornerIndex);
		
		IntVector roomDistance = GetDistanceOffsetBetweenRooms(frame.origin.room, corner_id.room, frame.invY);
		Corner result(rawCorner);

		result.pos = GetDistanceBetween(frame.origin, GlobalPosition(corner_id.room, rawCorner.pos), frame.invY);
		
		if (frame.invX) {
			switch (result.type) {
				case TopLeft:
					result.type = TopRight;
					break;
				case TopRight:
					result.type = TopLeft;
					break;
				case BottomLeft:
					result.type = BottomRight;
					break;
				case BottomRight:
					result.type = BottomLeft;
					break;
			}
		}
		if (frame.invY) {
			switch (result.type) {
				case TopLeft:
					result.type = BottomLeft;
					break;
				case TopRight:
					result.type = TopRight;
					break;
				case BottomLeft:
					result.type = TopLeft;
					break;
				case BottomRight:
					result.type = TopRight;
					break;
			}
		}

		return result;
	}

	RoomWall& GetWall(WallID wall_id) {
		return GetRoomData(wall_id.room).walls.at(wall_id.wallIndex);
	}
	RoomWall GetWallInLocalFrame(WallID wall_id, const LocalFrame& frame) {
		RoomData& roomData = GetRoomData(wall_id.room);
		RoomWall& rawWall = roomData.walls.at(wall_id.wallIndex);
		bool isHorizontal = rawWall.type == Ceiling || rawWall.type == Floor;

		IntVector roomDistance = GetDistanceOffsetBetweenRooms(frame.origin.room, wall_id.room, frame.invY);

		RoomWall result(rawWall);

		if (isHorizontal) {
			result.plane = rawWall.plane - frame.origin.pos.y + roomDistance.y;
			result.min = rawWall.min - frame.origin.pos.x + roomDistance.x;
			result.max = rawWall.max - frame.origin.pos.x + roomDistance.x;
		} else {
			result.plane = rawWall.plane - frame.origin.pos.x + roomDistance.x;
			result.min = rawWall.min - frame.origin.pos.y + roomDistance.y;
			result.max = rawWall.max - frame.origin.pos.y + roomDistance.y;
		}

		if (frame.invX) {
			if (result.type == LeftWall) {
				result.type = RightWall;
			} else if (result.type == RightWall) {
				result.type = LeftWall;
			} else {
				int tmp = result.min;
				result.min = -result.max;
				result.max = -tmp;
				bool tmp2 = result.minCornerConcave;
				result.minCornerConcave = result.maxCornerConcave;
				result.maxCornerConcave = tmp2;
			}
		}
		if (frame.invY) {
			if (result.type == Ceiling) {
				result.type = Floor;
			} else if (result.type == Floor) {
				result.type = Ceiling;
			} else {
				int tmp = result.min;
				result.min = -result.max;
				result.max = -tmp;
				bool tmp2 = result.minCornerConcave;
				result.minCornerConcave = result.maxCornerConcave;
				result.maxCornerConcave = tmp2;
			}
		}

		return result;
	}

	GravityLine& GetGravityLine(LineID line_id) {
		return GetRoomData(line_id.room).lines.at(line_id.lineIndex);
	}
	GravityLine GetGravityLineInLocalFrame(LineID line_id, const LocalFrame& frame) {
		RoomData& roomData = GetRoomData(line_id.room);
		GravityLine& rawLine = roomData.lines.at(line_id.lineIndex);

		IntVector roomDistance = GetDistanceOffsetBetweenRooms(frame.origin.room, line_id.room, frame.invY);

		GravityLine result(rawLine);
		result.min = rawLine.min - frame.origin.pos + roomDistance;
		result.max = rawLine.max - frame.origin.pos + roomDistance;

		if (frame.invX) {
			int tmp = result.min.x;
			result.min.x = -result.max.x;
			result.max.x = -tmp;
		}
		if (frame.invY) {
			int tmp = result.min.y;
			result.min.y = -result.max.y;
			result.max.y = -tmp;
		}

		return result;
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

	// Important: this function must be SOUND
	// That is, it should only say an edge is *impossible* if it verifiably is - otherwise, the heuristic might become inadmissible
	// When in doubt, we should assume that it is possible
	bool IsEdgePossibleWithoutFlipping(NavigationEdge& edge) {
		NavigationNode& from_node = GetNavigationNode(edge.from);
		NavigationNode& to_node = GetNavigationNode(edge.to);

		bool startInverseGravity = false;
		GlobalPosition fromNodePos(edge.from.room, IntVector(0, 0));
		int fromVGap = 0;
		IntVector fromPosMin;
		IntVector fromPosMax;
		switch (from_node.type) {
			default:
				return false;
			case StartNodeType:
				startInverseGravity = from_node.data.start.inverseGravity;
				fromPosMin = from_node.data.start.pos;
				fromPosMax = from_node.data.start.pos;
				fromNodePos = GlobalPosition(edge.from.room, from_node.data.start.pos);
				break;
			case CornerNodeType:
			{
				Corner& fromCorner = GetCorner(from_node.data.corner.corner);
				startInverseGravity = from_node.data.corner.inverseGravity;
				fromNodePos = GlobalPosition(edge.from.room, fromCorner.pos);
				fromVGap = (fromCorner.type == TopLeft || fromCorner.type == TopRight) ? (1 - fromCorner.verticalGap) : (fromCorner.verticalGap - 1);
				switch (fromCorner.type) {
					default:
						return false;
					case TopLeft:
					case TopRight:
						fromPosMin = IntVector(fromCorner.pos.x, fromCorner.pos.y - fromCorner.verticalGap + 1);
						fromPosMax = fromCorner.pos;
						break;
					case BottomLeft:
					case BottomRight:
						fromPosMin = fromCorner.pos;
						fromPosMax = IntVector(fromCorner.pos.x, fromCorner.pos.y + fromCorner.verticalGap - 1);
						break;
				}
				break;
			}
		}

		bool gravityChange = false;
		GlobalPosition toNodePos(edge.to.room, IntVector(0, 0));
		int toVGap = 0;
		IntVector toPosMin;
		IntVector toPosMax;
		switch (to_node.type) {
			default:
				return false;
			case GoalNodeType:
				gravityChange = false;
				toPosMin = to_node.data.goal.pos;
				toPosMax = to_node.data.goal.pos;
				toNodePos = GlobalPosition(edge.to.room, to_node.data.goal.pos);
				break;
			case CornerNodeType:
			{
				Corner& toCorner = GetCorner(to_node.data.corner.corner);
				gravityChange = (to_node.data.corner.inverseGravity != startInverseGravity);
				toNodePos = GlobalPosition(edge.to.room, toCorner.pos);
				toVGap = (toCorner.type == TopLeft || toCorner.type == TopRight) ? (1 - toCorner.verticalGap) : (toCorner.verticalGap - 1);
				switch (toCorner.type) {
					default:
						return false;
					case TopLeft:
					case TopRight:
						toPosMin = IntVector(toCorner.pos.x, toCorner.pos.y - toCorner.verticalGap + 1);
						toPosMax = toCorner.pos;
						break;
					case BottomLeft:
					case BottomRight:
						toPosMin = toCorner.pos;
						toPosMax = IntVector(toCorner.pos.x, toCorner.pos.y + toCorner.verticalGap - 1);
						break;
				}
				break;
			}
		}
		// Gravity change trivially implies a flip is needed
		if (gravityChange) {
			return false;
		}

		bool invX = edge.distance.x < 0;
		bool invY = startInverseGravity;
		GlobalPosition referencePos(edge.from.room, invY ? fromPosMax : fromPosMin);
		LocalFrame localFrame(referencePos, invX, invY);

		IntVector fromPosMinLocal = ToLocalCoords(localFrame, GlobalPosition(edge.from.room, invY ? fromPosMax : fromPosMin));
		IntVector fromPosMaxLocal = ToLocalCoords(localFrame, GlobalPosition(edge.from.room, invY ? fromPosMin : fromPosMax));
		IntVector toPosMinLocal = ToLocalCoords(localFrame, GlobalPosition(edge.to.room, invY ? toPosMax : toPosMin));
		IntVector toPosMaxLocal = ToLocalCoords(localFrame, GlobalPosition(edge.to.room, invY ? toPosMin : toPosMax));

		int x_distance = toPosMinLocal.x;
		int max_y_distance = toPosMaxLocal.y - fromPosMinLocal.y;
		if (max_y_distance < 0) {
			// Shouldn't happen but just in case
			return false;
		}

		// The edge is "impossible" if the number of frames to traverse it horizontally is greater than the number of frames to traverse it vertically
		// Note however that horizontal movement happens first - so we sort of get an extra frame of horizontal movement in close calls
		// (abs_d + maxHSpeed - 1) / maxHSpeed - 1 can be simplified to:
		int hFrames = GetMinXFrames(x_distance);
		int vFrames = GetMaxYFrames(max_y_distance);

		// Edge is possible (possibly only with ideal starting conditions)
		if (hFrames <= vFrames + 1) {
			return true;
		}

		// Try to find a sequence of walkable surfaces that let you get to the destination
		std::set<RoomPosition> rooms_1 = GetTouchedRooms(fromNodePos, toNodePos, edge.distance.y < 0);
		std::set<RoomPosition> rooms_2;
		
		// Just in case gaps go across screen boundaries:
		// This is technically not sound but probably is good enough
		if (fromVGap != 0 || toVGap != 0) {
			GlobalPosition fromNodeGapPos(fromNodePos);
			GlobalPosition toNodeGapPos(toNodePos);
			fromNodeGapPos.pos.y += fromVGap;
			toNodeGapPos.pos.y += toVGap;
			fromNodeGapPos = PlayerRoomChangeLogic(fromNodeGapPos);
			toNodeGapPos = PlayerRoomChangeLogic(toNodeGapPos);
			rooms_2 = GetTouchedRooms(fromNodeGapPos, toNodeGapPos, edge.distance.y + toVGap - fromVGap < 0);	
		}

		std::vector<RoomPosition> rooms;
		std::set_union(rooms_1.begin(), rooms_1.end(), rooms_2.begin(), rooms_2.end(), std::inserter(rooms, rooms.begin()));

		IntVector edgeStartLocal = ToLocalCoords(localFrame, GlobalPosition(fromNodePos.room, fromNodePos.pos));
		IntVector edgeEndLocal = ToLocalCoords(localFrame, GlobalPosition(toNodePos.room, toNodePos.pos));

		// Find all valid surfaces in the rooms
		std::vector<RoomWall> surfaces;
		for (int r = 0; r < rooms.size(); r++) {
			RoomPosition& room = rooms.at(r);
			RoomData& roomData = GetRoomData(room);

			int num_walls = roomData.walls.size();
			for (int w = 0; w < num_walls; w++) {
				RoomWall& wall = roomData.walls.at(w);
				if (startInverseGravity && wall.type != WallType::Ceiling) {
					continue;
				}
				if (!startInverseGravity && wall.type != WallType::Floor) {
					continue;
				}
				
				IntVector minCornerLocal = ToLocalCoords(localFrame, GlobalPosition(room, IntVector(wall.min, wall.plane)));
				IntVector maxCornerLocal = ToLocalCoords(localFrame, GlobalPosition(room, IntVector(wall.max, wall.plane)));

				RoomWall tmp_wall;
				tmp_wall.type = wall.type;
				tmp_wall.walkable = wall.walkable;
				tmp_wall.minCornerConcave = wall.minCornerConcave;
				tmp_wall.maxCornerConcave = wall.maxCornerConcave;
				tmp_wall.min = SDL_min(minCornerLocal.x, maxCornerLocal.x);
				tmp_wall.max = SDL_max(minCornerLocal.x, maxCornerLocal.x);
				tmp_wall.plane = minCornerLocal.y;

				// Wall is outside horizontal or vertical range of edge
				if (tmp_wall.plane < 0 || tmp_wall.plane > max_y_distance) {
					continue;
				} else if (tmp_wall.max < 0 || tmp_wall.min > x_distance) {
					continue;
				}

				// Only add surfaces that the edge goes over (not underneath)
				bool edgeGoesOverSurface;
				if (tmp_wall.min < 0) {
					edgeGoesOverSurface = true;
				} else if (tmp_wall.min > x_distance) {
					edgeGoesOverSurface = false;
				} else {
					bool minCornerIsBelow = (edgeEndLocal.x - edgeStartLocal.x) * (tmp_wall.plane - edgeEndLocal.y) >= (edgeEndLocal.y - edgeStartLocal.y) * (tmp_wall.min - edgeEndLocal.x);

					if (minCornerIsBelow) {
						edgeGoesOverSurface = true;
					} else {
						edgeGoesOverSurface = false;
					}
				}

				if (edgeGoesOverSurface) {
					surfaces.emplace_back(tmp_wall);
				}
			}
		}

		// Sort the surfaces by Y plane and min X pos
		std::function<bool(RoomWall, RoomWall)> cmp = [](const RoomWall& a, const RoomWall& b)
			{
				return (a.plane == b.plane) ? (a.min < b.min) : (a.plane < b.plane);
			};
		std::sort(surfaces.begin(), surfaces.end(), cmp);

		// Make sure we can go over all surfaces and end up high enough
		IntVector currentPos = IntVector(0, 0);
		for (int s = 0; s < surfaces.size(); s++) {
			RoomWall& surface = surfaces.at(s);

			int d_y = surface.plane - currentPos.y;
			int d_x_min = surface.min - currentPos.x;
			int d_x_max = surface.max - currentPos.x;
			
			if (d_y < 0) {
				// We are already past this surface
				// This shouldn't actually happen due to sorting
				VVV_exit(-1);
				continue;
			}

			// How far can we move before we touch the surface
			int frames = GetMaxYFrames(d_y);
			int max_x_dist = frames * MAX_X_SPEED;

			// The position we'd land on the platform at
			IntVector landingPos = IntVector(currentPos.x + max_x_dist, surface.plane);

			bool landingIsPossible = landingPos.x >= surface.min && surface.walkable && !surface.maxCornerConcave;
			bool landingIsForced = landingPos.x < surface.max;
			bool landingIsBeneficial = MAX_X_SPEED * surface.plane - MAX_Y_SPEED * surface.max < MAX_X_SPEED * currentPos.y - MAX_Y_SPEED * currentPos.x;

			if (landingIsForced && !landingIsPossible) {
				return false;
			}
			if (landingIsPossible && (landingIsForced || landingIsBeneficial)) {
				currentPos.x = surface.max;
				currentPos.y = surface.plane;

				// Check if we can reach the end from where we ended up
				int x_frames = GetMinXFrames(x_distance - currentPos.x);
				int y_frames = GetMaxYFrames(max_y_distance - currentPos.y);

				if (x_frames <= y_frames + 1) {
					return true;
				} else if (currentPos.y > max_y_distance) {
					return false;
				}
			}
		}

		// Check if we can reach the end from where we ended up
		int x_frames = GetMinXFrames(x_distance - currentPos.x);
		int y_frames = GetMaxYFrames(max_y_distance - currentPos.y);

		return x_frames <= y_frames + 1;
	}

	// Check if edges are geometrically crossed, as well as having same gravity direction
	bool DoEdgesCross(NavigationEdge& e1, NavigationEdge& e2) {
		NavigationNode& e1_from = GetNavigationNode(e1.from);
		NavigationNode& e1_to = GetNavigationNode(e1.to);
		NavigationNode& e2_from = GetNavigationNode(e2.from);
		NavigationNode& e2_to = GetNavigationNode(e2.to);

		RoomPosition e1_room;
		IntVector e1_origin;
		IntVector e1_dir = e1.distance;
		bool e1_invGravity = -1;

		RoomPosition e2_room;
		IntVector e2_origin;
		IntVector e2_dir = e2.distance;
		bool e2_invGravity;
		switch (e1_from.type) {
			default:
				return false;
			case StartNodeType:
				e1_room = e1_from.data.start.room;
				e1_origin = e1_from.data.start.pos;
				e1_invGravity = e1_from.data.start.inverseGravity;
				break;
			case GoalNodeType:
				e1_room = e1_from.data.goal.room;
				e1_origin = e1_from.data.goal.pos;
				e1_invGravity = e1.distance.y < 0;
				break;
			case CornerNodeType:
			{
				Corner& c = GetCorner(e1_from.data.corner.corner);
				e1_room = e1_from.data.corner.corner.room;
				e1_origin = c.pos;
				e1_invGravity = e1_from.data.corner.inverseGravity;
				break;
			}
		}
		switch (e2_from.type) {
			default:
				return false;
			case StartNodeType:
				e2_room = e2_from.data.start.room;
				e2_origin = e2_from.data.start.pos;
				e2_invGravity = e2_from.data.start.inverseGravity;
				break;
			case GoalNodeType:
				e2_room = e2_from.data.goal.room;
				e2_origin = e2_from.data.goal.pos;
				e2_invGravity = e2.distance.y < 0;
				break;
			case CornerNodeType:
			{
				Corner& c = GetCorner(e2_from.data.corner.corner);
				e2_room = e2_from.data.corner.corner.room;
				e2_origin = c.pos;
				e2_invGravity = e2_from.data.corner.inverseGravity;
				break;
			}
		}

		if (e1_invGravity != e2_invGravity) {
			return false;
		}

		// Edge in same direction
		if (IsSameOrInverseNode(e1.from, e2.from) && IsSameOrInverseNode(e1.to, e2.to)) {
			return true;
		}
		// Edge in opposite direction
		if (IsSameOrInverseNode(e1.from, e2.to) && IsSameOrInverseNode(e2.from, e1.to)) {
			return true;
		}

		int d_rx = GetHOffsetBetweenRooms(e1_room, e2_room);

		int d_ry_base = e2_room.ry - e1_room.ry;
		for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
			IntVector e2_offset = IntVector(d_rx * 320, d_ry * 240);
			IntVector e2_origin_offset = IntVector(e2_offset.x + e2_origin.x - e1_origin.x, e2_offset.y + e2_origin.y - e1_origin.y);

			int dir_cross_prod = e1_dir.x * e2_dir.y - e1_dir.y * e2_dir.x;
			int d1_o2_cross_prod = e1_dir.y * e2_origin_offset.x - e1_dir.x * e2_origin_offset.y;
			int d2_o2_cross_prod = e2_dir.y * e2_origin_offset.x - e2_dir.x * e2_origin_offset.y;
		
			int term_A = dir_cross_prod;
			int term_B = d2_o2_cross_prod;
			int term_C = dir_cross_prod;
			int term_D = d1_o2_cross_prod;

			if ((term_A < 0) != (term_B < 0)) {
				// t_1 < 0 -> no crossing
				continue;
			}
			if (std::abs(term_A) < std::abs(term_B)) {
				// t_1 > 1 -> no crossing
				continue;
			}
			if ((term_C < 0) != (term_D < 0)) {
				// t_2 < 0 -> no crossing
				continue;
			}
			if (std::abs(term_C) < std::abs(term_D)) {
				// t_2 > 1 -> no crossing
				continue;
			}

			// Special cases:
			if (term_B == 0 && term_D != 0 || term_B != 0 && term_D == 0) {
				// t_1 = t_2 = 0 -> crossing
				continue;
			}
			if (std::abs(term_A) != std::abs(term_B) && std::abs(term_C) == std::abs(term_D)) {
				// t_1 = t_2 = 1 -> crossing
				continue;
			}
			if (std::abs(term_A) == std::abs(term_B) && std::abs(term_C) != std::abs(term_D)) {
				// t_1 = t_2 = 1 -> crossing
				continue;
			}
			if ((term_B == 0 || term_D == 0) && (term_A == 0 || term_C == 0)) {
				// Edges lie on the same line -> special logic needed
				if (std::abs(e1_dir.y) > std::abs(e1_dir.x)) {
					// dy can't be zero
					if ((e1_dir.y < 0) != (e2_dir.y < 0)) {
						// Edges point in opposite directions (so they can add together)
						if ((e1_dir.y < 0) != (e2_origin_offset.y < 0)) {
							// Both edges point away from each others origins
							continue;
						} else if (e1_dir.y - e2_dir.y < e2_origin_offset.y) {
							// Both edges combined can't reach other origin -> no intersection
							continue;
						}
					} else {
						// Edges point in same direction
						if ((e1_dir.y < 0) != (e2_origin_offset.y < 0)) {
							// Edge 1 points away from second's origin
							if (e2_dir.y < e2_origin_offset.y) {
								// Second edge is farther away than it's length -> no intersection
								continue;
							}
						} else if (e1_dir.y < e2_origin_offset.y) {
							// First edge can't reach other origin -> no intersection
							continue;
						}
					}
				} else {
					// dx can't be zero
					if ((e1_dir.x < 0) != (e2_dir.x < 0)) {
						// Edges point in opposite directions (so they can add together)
						if ((e1_dir.x < 0) != (e2_origin_offset.x < 0)) {
							// Both edges point away from each others origins
							continue;
						}
						else if (e1_dir.x - e2_dir.x < e2_origin_offset.x) {
							// Both edges combined can't reach other origin -> no intersection
							continue;
						}
					}
					else {
						// Edges point in same direction
						if ((e1_dir.x < 0) != (e2_origin_offset.x < 0)) {
							// Edge 1 points away from second's origin
							if (e2_dir.x < e2_origin_offset.x) {
								// Second edge is farther away than it's length -> no intersection
								continue;
							}
						} else if (e1_dir.x < e2_origin_offset.x) {
							// First edge can't reach other origin -> no intersection
							continue;
						}
					}
				}
			}

			float t_1 = ((float)term_B) / ((float)term_A);
			float t_2 = ((float)term_D) / ((float)term_C);

			return true;
		}

		return false;
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

		uint8_t* collision_bitmap = GetCurrentRoomPlayerCollisionBitmap(min, max);
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
				bool left = collision_bitmap[(y - min.y) * bitmap_width + (x - 1 - min.x)] > 0;
				bool self = collision_bitmap[(y - min.y) * bitmap_width + (x - min.x)] > 0;

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
			uint8_t started = false;
			bool isCeiling = false;
			bool prevIsEmpty = true;
			bool startIsConcave = false;
			int wall_start = INT_MIN;

			for (int x = min.x; x <= max.x; x++) {
				uint8_t up_kind = collision_bitmap[bitmap_width * (y - min.y - 1) + (x - min.x)];
				uint8_t self_kind = collision_bitmap[bitmap_width * (y - min.y) + (x - min.x)];

				bool up = up_kind != 0;
				bool self = self_kind != 0;

				bool isWall = up != self;
				bool newWallIsCeiling = up && !self;
				bool isSameWall = isCeiling == newWallIsCeiling;

				bool wallEndsHere = (started != 0) && !(isWall && isSameWall);
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
					newWall.walkable = (started == 1);

					result.walls.push_back(newWall);
					started = 0;
				}
				if (wallStartsHere) {
					isCeiling = newWallIsCeiling;
					startIsConcave = !prevIsEmpty;
					wall_start = (x - 1);
					started = std::max(up_kind, self_kind);
				}
				prevIsEmpty = !up && !self;
			}

			// Finish off any running walls
			if (started != 0) {
				RoomWall newWall;
				newWall.type = isCeiling ? WallType::Ceiling : WallType::Floor;
				// wall.plane is the last coordinate the player can stand at just next to the wall
				newWall.plane = isCeiling ? y : (y - 1);
				newWall.min = wall_start;
				newWall.max = max.x;
				newWall.minCornerConcave = startIsConcave;
				newWall.maxCornerConcave = false;
				newWall.walkable = (started == 1);

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

				int corner_x = (up_left || left) ? x : (x - 1);
				int corner_y = (up_left || up) ? y : (y - 1);

				// Let's measure the vertical and horizontal gaps
				int verticalGap = 0;
				int horizontalGap = 0;

				int x_increment = (up_left || left) ? 1 : -1;
				int y_increment = (up_left || up) ? 1 : -1;

				int gap_x;
				for (gap_x = corner_x; true; gap_x += x_increment) {
					bool gap_up = collision_bitmap[bitmap_width * (y - min.y - 1) + (gap_x - min.x)];
					bool gap_self = collision_bitmap[bitmap_width * (y - min.y) + (gap_x - min.x)];

					if (gap_up || gap_self) {
						break;
					}
					horizontalGap++;
					if (gap_x <= min.x || gap_x >= max.x) {
						horizontalGap = -1;
						break;
					}
				}
				int gap_y;
				for (gap_y = corner_y; true; gap_y += y_increment) {
					bool gap_left = collision_bitmap[bitmap_width * (gap_y - min.y) + (x - min.x - 1)];
					bool gap_self = collision_bitmap[bitmap_width * (gap_y - min.y) + (x - min.x)];

					if (gap_left || gap_self) {
						break;
					}
					verticalGap++;
					if (gap_y <= min.y || gap_y >= max.y) {
						verticalGap = -1;
						break;
					}
				}

				bool simple = true;
				// Check walls are continuous
				for (int region_x = SDL_min(gap_x, corner_x); simple && region_x <= SDL_max(gap_x, corner_x); region_x++) {
					bool region_border = collision_bitmap[bitmap_width * (gap_y - min.y) + (region_x - min.x)];
					if (!region_border) {
						simple = false;
					}
				}
				for (int region_y = SDL_min(gap_y, corner_y); simple && region_y <= SDL_max(gap_y, corner_y); region_y++) {
					bool region_border = collision_bitmap[bitmap_width * (region_y - min.y) + (gap_x - min.x)];
					if (!region_border) {
						simple = false;
					}
				}
				gap_x -= x_increment;
				gap_y -= y_increment;
				// Check region is empty
				for (int region_x = SDL_min(gap_x, corner_x); simple && region_x <= SDL_max(gap_x, corner_x); region_x++) {
					for (int region_y = SDL_min(gap_y, corner_y); simple && region_y <= SDL_max(gap_y, corner_y); region_y++) {
						bool region_area = collision_bitmap[bitmap_width * (region_y - min.y) + (region_x - min.x)];
						if (region_area) {
							simple = false;
						}
					}
				}

				// More gaps to be measured
				int verticalNegativeGap = 0;
				int horizontalNegativeGap = 0;
				for (gap_x = corner_x; true; gap_x -= x_increment) {
					bool gap_self = collision_bitmap[bitmap_width * (corner_y - min.y) + (gap_x - min.x)];

					if (gap_self) {
						break;
					}
					horizontalNegativeGap++;
					if (gap_x <= min.x || gap_x >= max.x) {
						horizontalNegativeGap = -1;
						break;
					}
				}
				for (gap_y = corner_y; true; gap_y -= y_increment) {
					bool gap_self = collision_bitmap[bitmap_width * (gap_y - min.y) + (corner_x - min.x)];

					if (gap_self) {
						break;
					}
					verticalNegativeGap++;
					if (gap_y <= min.y || gap_y >= max.y) {
						verticalNegativeGap = -1;
						break;
					}
				}

				// Lastly, create the actual corner struct
				Corner newCorner;
				newCorner.pos.x = corner_x;
				newCorner.pos.y = corner_y;
				newCorner.horizontalGap = horizontalGap;
				newCorner.verticalGap = verticalGap;
				newCorner.horizontalNegativeGap = horizontalNegativeGap;
				newCorner.verticalNegativeGap = verticalNegativeGap;
				newCorner.simple = simple;
				if (up_left) {
					newCorner.type = BottomRight;
				} else if (left) {
					newCorner.type = TopRight;
				} else if (up) {
					newCorner.type = BottomLeft;
				} else {
					newCorner.type = TopLeft;
				}

				result.corners.push_back(newCorner);
			}
		}

		// Free the collision bitmap
		SDL_free((void*) collision_bitmap);

		// Measure screen-crossing gaps
		int num_corners = result.corners.size();
		for (int c = 0; c < num_corners; c++) {
			Corner& corner = result.corners.at(c);

			GlobalPosition cornerPos(room_pos, corner.pos);

			if (corner.horizontalGap == -1) {
				int gapSize = 0;

				IntVector offset, shift;
				switch (corner.type) {
					case TopLeft:
						offset = IntVector(0, 1);
						shift = IntVector(-1, 0);
						break;
					case BottomLeft:
						offset = IntVector(0, -1);
						shift = IntVector(-1, 0);
						break;
					case TopRight:
						offset = IntVector(0, 1);
						shift = IntVector(1, 0);
						break;
					case BottomRight:
						offset = IntVector(0, -1);
						shift = IntVector(1, 0);
						break;
				}

				GlobalPosition currentPos = cornerPos;
				while (true) {
					if (GetPlayerCollisionAt(currentPos) != 0) {
						break;
					}
					currentPos.pos.x += offset.x;
					currentPos.pos.y += offset.y;
					if (GetPlayerCollisionAt(currentPos) != 0) {
						break;
					}
					currentPos.pos.x -= offset.x;
					currentPos.pos.y -= offset.y;

					currentPos.pos.x += shift.x;
					currentPos.pos.y += shift.y;
					currentPos = PlayerRoomChangeLogic(currentPos);
					gapSize++;
				}

				corner.horizontalGap = gapSize;
			}
			if (corner.verticalGap == -1) {
				int gapSize = 0;

				IntVector offset, shift;
				switch (corner.type) {
				case TopLeft:
					offset = IntVector(1, 0);
					shift = IntVector(0, -1);
					break;
				case TopRight:
					offset = IntVector(-1, 0);
					shift = IntVector(0, -1);
					break;
				case BottomLeft:
					offset = IntVector(1, 0);
					shift = IntVector(0, 1);
					break;
				case BottomRight:
					offset = IntVector(-1, 0);
					shift = IntVector(0, 1);
					break;
				}

				GlobalPosition currentPos = cornerPos;
				while (true) {
					if (GetPlayerCollisionAt(currentPos) != 0) {
						break;
					}
					currentPos.pos.x += offset.x;
					currentPos.pos.y += offset.y;
					if (GetPlayerCollisionAt(currentPos) != 0) {
						break;
					}
					currentPos.pos.x -= offset.x;
					currentPos.pos.y -= offset.y;

					currentPos.pos.x += shift.x;
					currentPos.pos.y += shift.y;
					currentPos = PlayerRoomChangeLogic(currentPos);
					gapSize++;
				}

				corner.verticalGap = gapSize;
			}
		}
		// Measure screen-crossing negative gaps
		for (int c = 0; c < num_corners; c++) {
			Corner& corner = result.corners.at(c);

			GlobalPosition cornerPos(room_pos, corner.pos);

			if (corner.horizontalNegativeGap == -1) {
				int gapSize = 0;

				IntVector shift;
				switch (corner.type) {
				case TopLeft:
					shift = IntVector(1, 0);
					break;
				case BottomLeft:
					shift = IntVector(1, 0);
					break;
				case TopRight:
					shift = IntVector(-1, 0);
					break;
				case BottomRight:
					shift = IntVector(-1, 0);
					break;
				}

				GlobalPosition currentPos = cornerPos;
				while (GetPlayerCollisionAt(currentPos) == 0) {
					currentPos.pos.x += shift.x;
					currentPos.pos.y += shift.y;
					currentPos = PlayerRoomChangeLogic(currentPos);
					gapSize++;
				}

				corner.horizontalNegativeGap = gapSize;
			}
			if (corner.verticalNegativeGap == -1) {
				int gapSize = 0;

				IntVector shift;
				switch (corner.type) {
				case TopLeft:
					shift = IntVector(0, 1);
					break;
				case TopRight:
					shift = IntVector(0, 1);
					break;
				case BottomLeft:
					shift = IntVector(0, -1);
					break;
				case BottomRight:
					shift = IntVector(0, -1);
					break;
				}

				GlobalPosition currentPos = cornerPos;
				while (GetPlayerCollisionAt(currentPos) == 0) {
					currentPos.pos.x += shift.x;
					currentPos.pos.y += shift.y;
					currentPos = PlayerRoomChangeLogic(currentPos);
					gapSize++;
				}

				corner.verticalNegativeGap = gapSize;
			}
		}
		// Load the initial room again
		LoadRoom(room_pos);

		// Now, let's store all the gravity lines
		int num_entities = obj.entities.size();
		for (int e = 0; e < num_entities; e++) {
			entclass& entity = obj.entities.at(e);

			bool isHorizontal;
			IntVector min;
			IntVector max;
			if (entity.type == 9 && entity.rule == 4 && entity.size == 5) {
				// Horizontal gravity line
				isHorizontal = true;
				int width = entity.w;
				// height = 1;
				IntVector pos(entity.xp, entity.yp);

				// The hitbox is a bit special - if you collided with the line on the previous frame, it still counts
				// But this is only relevant because of the cooldown, since you can't tunnel through the line (without zipping)
				min.x = pos.x - VIRIDIAN_CX - VIRIDIAN_W;
				max.x = pos.x + width - VIRIDIAN_CX;

				min.y = pos.y - VIRIDIAN_H;
				max.y = pos.y - 1; // Touching with equality is below the line
			} else if (entity.type == 10 && entity.rule == 5 && entity.size == 6) {
				// Vertical gravity line
				isHorizontal = false;
				// width = 1;
				int height = entity.h;
				IntVector pos(entity.xp, entity.yp);

				min.y = pos.y - VIRIDIAN_CY - VIRIDIAN_H;
				max.y = pos.y + height - VIRIDIAN_CY;

				min.x = pos.x - 1 - VIRIDIAN_CX - VIRIDIAN_W;
				max.x = pos.x - 1 - VIRIDIAN_CX - 1; // Touching with equality is past the line
			} else {
				continue;
			}

			result.lines.emplace_back(isHorizontal, min, max);
		}
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
			start_node = NavigationNodeID(r, roomData.nodes.size());
			roomData.nodes.emplace_back(startNode);
		}
		if (r == goal_room) {
			GoalNavigationNode goalNode(goal_room, IntVector(goal_x, goal_y));
			goal_node = NavigationNodeID(r, roomData.nodes.size());
			roomData.nodes.emplace_back(goalNode);
		}

		int num_corners = roomData.corners.size();
		int num_walls = roomData.walls.size();

		for (int c_idx = 0; c_idx < num_corners; c_idx++) {
			Corner& c = roomData.corners.at(c_idx);

			CornerID c_id_1(r, c_idx, false);
			CornerID c_id_2(r, c_idx, true);

			NavigationNode n1(c_id_1, false);
			NavigationNode n2(c_id_2, true);

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
			int d_x = d_rx * 320 + cornerData.pos.x - s.pos.x;

			if (goingLeft && d_x > 0 || !goingLeft && d_x < 0) {
				return;
			}

			int d_ry_base = to.room.ry - from.room.ry;
			for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
				int d_y = d_ry * 240 + cornerData.pos.y - s.pos.y;

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
			int d_x = d_rx * 320 + g.pos.x - cornerData.pos.x;

			if (goingLeft && d_x > 0 || !goingLeft && d_x < 0) {
				return;
			}

			int d_ry_base = to.room.ry - from.room.ry;
			for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
				int d_y = d_ry * 240 + g.pos.y - cornerData.pos.y;

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

				Ray ray = Ray(cornerData.pos.x, cornerData.pos.y, d_x, d_y);
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
			int d_x = d_rx * 320 + c2_data.pos.x - c1_data.pos.x;

			if (goingLeft && d_x > 0 || goingRight && d_x < 0) {
				return;
			}

			int d_ry_base = to.room.ry - from.room.ry;
			for (int d_ry = d_ry_base - 20; d_ry <= d_ry_base + 20; d_ry += 20) {
				int d_y = d_ry * 240 + c2_data.pos.y - c1_data.pos.y;

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

				Ray ray = Ray(c1_data.pos.x, c1_data.pos.y, d_x, d_y);
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

	// TODO: finish this
	bool CanConnectCornersViaSurface(CornerID c1_id, WallID w_id, CornerID c2_id) {
		RoomWall& surface = GetWall(w_id);

		bool startInverseGravity = surface.type == WallType::Ceiling;
		bool endInverseGravity = surface.type == WallType::Floor;
		if (!startInverseGravity && !endInverseGravity) {
			return false;
		}
		
		RoomPosition fromRoom = c1_id.room;
		RoomPosition wallRoom = w_id.room;
		RoomPosition toRoom = c2_id.room;

		Corner& fromCorner = GetCorner(c1_id);
		Corner& toCorner = GetCorner(c2_id);

		bool startGoingRight;
			switch (fromCorner.type) {
			default:
				return false;
			case BottomLeft: case TopRight:
				startGoingRight = !startInverseGravity;
				break;
			case TopLeft: case BottomRight:
				startGoingRight = startInverseGravity;
				break;
		}
		bool endGoingRight;
		switch (toCorner.type) {
			default:
				return false;
			case BottomLeft: case TopRight:
				endGoingRight = !endInverseGravity;
				break;
			case TopLeft: case BottomRight:
				endGoingRight = endInverseGravity;
				break;
		}

		// Treat fromCorner as origin for simplicity, get offsets to everything else
		int wall_d_rx = GetHOffsetBetweenRooms(fromRoom, wallRoom);
		int to_d_rx = GetHOffsetBetweenRooms(fromRoom, toRoom);

		int wall_d_ry_base = wallRoom.ry - fromRoom.ry;
		int to_d_ry_base = toRoom.ry - fromRoom.ry;
		
		for (int wall_d_ry = wall_d_ry_base - 20; wall_d_ry <= wall_d_ry_base + 20; wall_d_ry += 20) {
			IntVector wall_room_offset = IntVector(320 * wall_d_rx, 240 * wall_d_ry);

			int wall_dy = surface.plane - fromCorner.pos.y + wall_room_offset.y;
			int wall_dx_min = surface.min - fromCorner.pos.x + wall_room_offset.x;
			int wall_dx_max = surface.max - fromCorner.pos.x + wall_room_offset.x;

			// Is the wall on the wrong side of the first corner?
			if (startInverseGravity && wall_dy > 0) {
				continue;
			} else if (!startInverseGravity && wall_dy < 0) {
				continue;
			} else if (startGoingRight && wall_dx_max < 0) {
				continue;
			} else if (!startGoingRight && wall_dx_min > 0) {
				continue;
			}

			int validWallRangeMin = wall_dx_min;
			int validWallRangeMax = wall_dx_max;
			if (startGoingRight) {
				validWallRangeMin = SDL_max(0, validWallRangeMin);
			} else {
				validWallRangeMax = SDL_min(0, validWallRangeMax);
			}
			
			for (int to_d_ry = to_d_ry_base - 20; to_d_ry <= to_d_ry_base + 20; to_d_ry += 20) {
				IntVector to_room_offset = IntVector(320 * to_d_rx, 240 * to_d_ry);
				IntVector wall_to_room_offset = IntVector(320 * (to_d_rx - wall_d_rx), 240 * (to_d_ry - wall_d_ry));

				// Distances to second corner from wall
				int to_dy = toCorner.pos.y - surface.plane + wall_to_room_offset.y;
				int to_dx_min = toCorner.pos.x - surface.min - wall_to_room_offset.x;
				int to_dx_max = toCorner.pos.x - surface.max - wall_to_room_offset.x;

				// Is the wall on the wrong side of the second corner?
				if (endInverseGravity && to_dy > 0) {
					continue;
				} else if (!endInverseGravity && to_dy < 0) {
					continue;
				} else if (endGoingRight && to_dx_min < 0) {
					continue;
				} else if (!endGoingRight && to_dx_max > 0) {
					continue;
				}

				if (endGoingRight) {
					validWallRangeMax = SDL_min(wall_dx_max + to_dx_max, validWallRangeMax);
				} else {
					validWallRangeMin = SDL_max(wall_dx_min + to_dx_min, validWallRangeMin);
				}

				if (validWallRangeMin > validWallRangeMax) {
					// No valid place to flip on the surface
					continue;
				}

				if (startGoingRight != endGoingRight) {
					// Only need to check the right- / leftmost pixel of the surface

				}
			}
		}
	}

	bool SurfaceIsVisibleFrom(GlobalPosition sourcePos, WallID w_id) {
		RoomWall& target = GetWall(w_id);

		bool invY;
		switch (target.type) {
			default:
				return false;
			case WallType::Ceiling:
				// Corner must be below the ceiling to see it
				invY = true;
				break;
			case WallType::Floor:
				// Corner must be above the floor to see it
				invY = false;
				break;
		}

		IntVector room_offset = GetDistanceOffsetBetweenRooms(sourcePos.room, w_id.room, invY);
		
		GlobalPosition targetMinPos = GlobalPosition(w_id.room, IntVector(target.min + 1, target.plane));
		GlobalPosition targetMaxPos = GlobalPosition(w_id.room, IntVector(target.max - 1, target.plane));

		bool invX;
		if (sourcePos.room.rx == targetMaxPos.room.rx) {
			invX = targetMaxPos.pos.x - sourcePos.pos.x < sourcePos.pos.x - targetMinPos.pos.x;
		} else {
			invX = targetMaxPos.room.rx < sourcePos.room.rx;
		}

		LocalFrame localFrame(sourcePos, invX, invY);

		// Get a list of all rooms between the corner and surfaces
		std::set<RoomPosition> rooms_1 = GetTouchedRooms(sourcePos, targetMinPos, invY);
		std::set<RoomPosition> rooms_2 = GetTouchedRooms(sourcePos, targetMaxPos, invY);

		std::vector<RoomPosition> rooms;
		std::set_union(rooms_1.begin(), rooms_1.end(), rooms_2.begin(), rooms_2.end(), std::inserter(rooms, rooms.begin()));

		// Construct local coordinate frame (corner at 0, 0, wall at positive coordinates)
		IntVector localTargetMin = ToLocalCoords(localFrame, GlobalPosition(targetMinPos.room, targetMinPos.pos));
		IntVector localTargetMax = ToLocalCoords(localFrame, GlobalPosition(targetMaxPos.room, targetMaxPos.pos));
		if (invX) {
			int tmp = localTargetMin.x;
			localTargetMin.x = localTargetMax.x;
			localTargetMax.x = tmp;
		}

		if (localTargetMin.y < 0) {
			localTargetMin.y += 240 * 20;
		}
		if (localTargetMax.y < 0) {
			localTargetMax.y += 240 * 20;
		}

		IntVector min = IntVector(SDL_min(0, localTargetMin.x), SDL_min(0, localTargetMin.y));
		IntVector max = IntVector(SDL_max(0, localTargetMax.x), SDL_max(0, localTargetMax.y));

		std::vector<RoomWall> surfaces;
		for (int r = 0; r < rooms.size(); r++) {
			RoomPosition room = rooms.at(r);
			RoomData& roomData = GetRoomData(room);
			for (int w = 0; w < roomData.walls.size(); w++) {
				RoomWall& wall = roomData.walls.at(w);

				// Wall faces away from corner
				if (invY && wall.type == WallType::Floor) {
					continue;
				} else if (!invY && wall.type == WallType::Ceiling) {
					continue;
				}

				// Don't include the target surface itself
				if (w == w_id.wallIndex && room == w_id.room) {
					continue;
				}

				GlobalPosition wallMin, wallMax;
				bool isHorizontalSurface;
				switch (wall.type) {
					default:
						continue;
					case WallType::Ceiling:
					case WallType::Floor:
						isHorizontalSurface = true;
						wallMin = GlobalPosition(room, IntVector(wall.min, wall.plane));
						wallMax = GlobalPosition(room, IntVector(wall.max, wall.plane));
						break;
					case WallType::LeftWall:
					case WallType::RightWall:
						isHorizontalSurface = false;
						wallMin = GlobalPosition(room, IntVector(wall.plane, wall.min));
						wallMax = GlobalPosition(room, IntVector(wall.plane, wall.max));
						break;
				}

				IntVector localWallMin = ToLocalCoords(localFrame, GlobalPosition(wallMin.room, wallMin.pos));
				IntVector localWallMax = ToLocalCoords(localFrame, GlobalPosition(wallMax.room, wallMax.pos));
				if (isHorizontalSurface) {
					if (localWallMin.y < 0) {
						localWallMin.y += 240 * 20;
					}
					if (localWallMax.y < 0) {
						localWallMax.y += 240 * 20;
					}
					if (invX) {
						int tmp = localWallMin.x;
						localWallMin.x = localWallMax.x;
						localWallMax.x = tmp;
					}
				}
				if (invY && !isHorizontalSurface) {
					int tmp = localWallMin.y;
					localWallMin.y = localWallMax.y;
					localWallMax.y = tmp;
				}

				if (localWallMin.x > max.x || localWallMax.x < min.x || localWallMin.y > max.y || localWallMax.y < min.y) {
					// Wall is outside useful bounds
					continue;
				}
				if (!isHorizontalSurface) {
					// Wall faces away from corner (so a different wall should be hit first)
					if (wall.type == WallType::RightWall && invX || wall.type == WallType::LeftWall && !invX) {
						if (localWallMin.x > 0) {
							continue;
						}
					} else {
						if (localWallMin.x < 0) {
							continue;
						}
					}
				}

				RoomWall localSurface(wall);
				localSurface.plane = isHorizontalSurface ? localWallMin.y : localWallMin.x;
				localSurface.min = isHorizontalSurface ? localWallMin.x : localWallMin.y;
				localSurface.max = isHorizontalSurface ? localWallMax.x : localWallMax.y;

				switch (localSurface.type) {
					default:
						continue;
					case WallType::Ceiling:
						if (invY) {
							localSurface.type = Floor;
						}
						if (invX) {
							int tmp = localSurface.min;
							localSurface.min = localSurface.max;
							localSurface.max = tmp;
						}
						break;
					case WallType::Floor:
						if (invY) {
							localSurface.type = Ceiling;
						}
						if (invX) {
							int tmp = localSurface.min;
							localSurface.min = localSurface.max;
							localSurface.max = tmp;
						}
						break;
					case WallType::LeftWall:
						if (invX) {
							localSurface.type = RightWall;
						}
						if (invY) {
							int tmp = localSurface.min;
							localSurface.min = localSurface.max;
							localSurface.max = tmp;
						}
						break;
					case WallType::RightWall:
						if (invX) {
							localSurface.type = LeftWall;
						}
						if (invY) {
							int tmp = localSurface.min;
							localSurface.min = localSurface.max;
							localSurface.max = tmp;
						}
						break;
				}


				surfaces.push_back(localSurface);
			}
		}

		if (surfaces.empty()) {
			return true;
		}

		Ray minCornerRay = Ray(0, 0, localTargetMin.x, localTargetMin.y);
		bool anyMinIntersection = false;
		for (int s = 0; s < surfaces.size(); s++) {
			float t = WallRayIntersection(surfaces.at(s), minCornerRay);
			if (0 <= t && t < 1) {
				anyMinIntersection = true;
				break;
			}
		}
		if (!anyMinIntersection) {
			// Min corner is visible
			return true;
		}

		Ray maxCornerRay = Ray(0, 0, localTargetMax.x, localTargetMax.y);
		bool anyMaxIntersection = false;
		for (int s = 0; s < surfaces.size(); s++) {
			float t = WallRayIntersection(surfaces.at(s), maxCornerRay);
			if (0 <= t && t < 1) {
				anyMaxIntersection = true;
				break;
			}
		}
		if (!anyMaxIntersection) {
			// Max corner is visible
			return true;
		}

		// TODO: fix this - only min and max corner are checked for visibility, not anything in between
		return false;
	}

	std::set<RoomPosition> GetTouchedRooms(GlobalPosition from, GlobalPosition to, bool invY) {
		std::set<RoomPosition> result;

		int d_rx = GetHOffsetBetweenRooms(from.room, to.room);
		int d_x = 320 * d_rx + to.pos.x - from.pos.x;
		int d_ry = invY ? GetMinNegativeVOffsetBetweenRooms(from.room, to.room) : GetMinPositiveVOffsetBetweenRooms(from.room, to.room);
		int d_y = 240 * d_ry + to.pos.y - from.pos.y;

		// Special case: d_y doesn't match invY (edge in same room but on wrong side
		if (invY && d_y > 0) {
			d_y -= 240 * 20;
		} else if (!invY && d_y < 0) {
			d_y += 240 * 20;
		}

		RoomPosition currentRoom = from.room;
		IntVector currentPos = from.pos;
		IntVector distance(d_x, d_y);

		// Basically a room-only raycast
		while (true) {
			RoomData& roomData = GetRoomData(currentRoom);

			// Find next intersection with room edge
			IntVector min = IntVector(roomData.GetMinXPos(), roomData.GetMinYPos());
			IntVector max = IntVector(roomData.GetMaxXPos(), roomData.GetMaxYPos());

			// AABB intersection
			float t_left = ((float)(min.x - currentPos.x)) / ((float)distance.x);
			float t_right = ((float)(max.x - currentPos.x)) / ((float)distance.x);
			float t_top = ((float)(min.y - currentPos.y)) / ((float)distance.y);
			float t_bottom = ((float)(max.y - currentPos.y)) / ((float)distance.y);

			float t_max = SDL_min(SDL_max(t_left, t_right), SDL_max(t_top, t_bottom));

			result.insert(currentRoom);

			// Edge ends in this room
			if (t_max > 1) {
				break;
			}

			// Change to next room
			if (t_top == t_max) {
				currentRoom = currentRoom.NextRoomUp();
				currentPos.y += 240;
			} else if (t_bottom == t_max) {
				currentRoom = currentRoom.NextRoomDown();
				currentPos.y -= 240;
			} else if (t_left == t_max) {
				currentRoom = currentRoom.NextRoomLeft();
				currentPos.x += 320;
			} else {
				currentRoom = currentRoom.NextRoomRight();
				currentPos.x -= 320;
			}
		}

		return result;
	}

	// Get the set of all surfaces directly vertically above this edge
	std::set<WallID> GetSurfacesAboveEdge(NavigationEdge& edge) {
		GlobalPosition fromNodePos = GetNodePos(edge.from);
		GlobalPosition toNodePos = GetNodePos(edge.to);

		bool invX = edge.distance.x < 0;
		if (invX) {
			GlobalPosition tmp = fromNodePos;
			fromNodePos = toNodePos;
			toNodePos = tmp;
		}

		IntVector distance = IntVector(std::abs(edge.distance.x), edge.distance.y);

		std::set<WallID> surfaces_above;
		int x = 0;
		while (x <= distance.x) {
			// Y should be rounded down to give first integer coordinate above the edge
			int max_y = edge.distance.y * x / edge.distance.x;

			GlobalPosition actualPos(fromNodePos);
			actualPos.pos += IntVector(x, max_y);
			actualPos = DoAllRoomChanges(actualPos);

			WallID next_surface(actualPos.room, -1);
			while (true) {
				RoomData& roomData = GetRoomData(actualPos.room);
				int best_y = 1000;
				for (int s = 0; s < roomData.walls.size(); s++) {
					RoomWall& wall = roomData.walls.at(s);
					if (wall.type != Ceiling) {
						// Not a ceiling
						continue;
					}
					if (wall.min > actualPos.pos.x || wall.max < actualPos.pos.x) {
						// Not above the desired x pos
						continue;
					}
					if (wall.plane <= actualPos.pos.y && wall.plane < best_y) {
						best_y = wall.plane;
						next_surface.room = actualPos.room;
						next_surface.wallIndex = s;
					}
				}

				if (next_surface.wallIndex > -1) {
					// Found it!
					break;
				} else {
					// Go to next room above
					actualPos.room = actualPos.room.NextRoomUp();
					actualPos.pos.y = roomData.GetMaxYPos() + 1;
				}
			}

			surfaces_above.insert(next_surface);

			RoomWall& s = GetWall(next_surface);
			// Go to next uncovered position
			x += s.max + 1 - actualPos.pos.x;
		}

		return surfaces_above;
	}

	// Get the set of all surfaces directly vertically below this edge
	std::set<WallID> GetSurfacesBelowEdge(NavigationEdge& edge) {
		GlobalPosition fromNodePos = GetNodePos(edge.from);
		GlobalPosition toNodePos = GetNodePos(edge.to);

		bool invX = edge.distance.x < 0;
		if (invX) {
			GlobalPosition tmp = fromNodePos;
			fromNodePos = toNodePos;
			toNodePos = tmp;
		}

		IntVector distance = IntVector(std::abs(edge.distance.x), edge.distance.y);

		std::set<WallID> surfaces_below;
		int x = 0;
		while (x <= distance.x) {
			// Y should be rounded up to give first integer coordinate below the edge
			int min_y = (edge.distance.y * x + edge.distance.x - 1) / edge.distance.x;

			GlobalPosition actualPos(fromNodePos);
			actualPos.pos += IntVector(x, min_y);
			actualPos = DoAllRoomChanges(actualPos);

			WallID next_surface(actualPos.room, -1);
			while (true) {
				RoomData& roomData = GetRoomData(actualPos.room);
				int best_y = -1000;
				for (int s = 0; s < roomData.walls.size(); s++) {
					RoomWall& wall = roomData.walls.at(s);
					if (wall.type != Floor) {
						// Not a floor
						continue;
					}
					if (wall.min > actualPos.pos.x || wall.max < actualPos.pos.x) {
						// Not below the desired x pos
						continue;
					}
					if (wall.plane >= actualPos.pos.y && wall.plane > best_y) {
						best_y = wall.plane;
						next_surface.room = actualPos.room;
						next_surface.wallIndex = s;
					}
				}

				if (next_surface.wallIndex > -1) {
					// Found it!
					break;
				}
				else {
					// Go to next room below
					actualPos.room = actualPos.room.NextRoomDown();
					actualPos.pos.y = roomData.GetMinYPos() - 1;
				}
			}

			surfaces_below.insert(next_surface);

			RoomWall& s = GetWall(next_surface);
			// Go to next uncovered position
			x += s.max + 1 - actualPos.pos.x;
		}

		return surfaces_below;
	}


	std::set<CornerID> FindCornersInRegion(GlobalPosition from, GlobalPosition to) {
		std::set<CornerID> result;

		IntVector roomOffset = GetMinOffsetBetweenRooms(from.room, to.room);
		IntVector range = GetMinDistanceBetween(from, to);

		const Region validRange = Region(IntVector::zero(), range);

		for (int rx = 0; rx <= roomOffset.x; rx++) {
			for (int ry = 0; ry <= roomOffset.y; ry++) {
				RoomPosition room((rx + from.room.rx) % 20, (ry + from.room.ry) % 20);
				RoomData& roomData = GetRoomData(room);

				IntVector roomDistance(rx * 320, ry * 240);

				for (int c = 0; c < roomData.corners.size(); c++) {
					Corner& corner = roomData.corners.at(c);

					IntVector relativePos = corner.pos + roomDistance - from.pos;

					if (validRange.contains(relativePos)) {
						result.emplace(room, c, false);
						result.emplace(room, c, true);
					}
				}
			}
		}

		return result;
	}
	std::set<CornerID> FindCornersInRelativeRegion(const LocalFrame& frame, const Region& region) {
		GlobalPosition min(frame.origin);
		GlobalPosition max(frame.origin);

		min.pos += region.getMin();
		max.pos += region.getMax();

		min = DoAllRoomChanges(min);
		max = DoAllRoomChanges(max);

		return FindCornersInRegion(min, max);
	}

	std::set<WallID> FindWallsInRegion(GlobalPosition from, GlobalPosition to) {
		std::set<WallID> result;

		IntVector roomOffset = GetMinOffsetBetweenRooms(from.room, to.room);
		IntVector range = GetMinDistanceBetween(from, to);
		const Region validRange = Region(IntVector::zero(), range);

		for (int rx = 0; rx <= roomOffset.x; rx++) {
			for (int ry = 0; ry <= roomOffset.y; ry++) {
				RoomPosition room((rx + from.room.rx) % 20, (ry + from.room.ry) % 20);
				RoomData& roomData = GetRoomData(room);

				IntVector roomDistance(rx * 320, ry * 240);

				for (int w = 0; w < roomData.walls.size(); w++) {
					RoomWall& wall = roomData.walls.at(w);

					IntVector minCorner = (wall.type == Floor || wall.type == Ceiling) ? IntVector(wall.min, wall.plane) : IntVector(wall.plane, wall.min);
					IntVector maxCorner = (wall.type == Floor || wall.type == Ceiling) ? IntVector(wall.max, wall.plane) : IntVector(wall.plane, wall.max);

					minCorner += roomDistance - from.pos;
					maxCorner += roomDistance - from.pos;

					const Region wallRegion = Region(minCorner, maxCorner);

					if (validRange.intersects(wallRegion)) {
						result.emplace(room, w);
					}
				}
			}
		}

		return result;
	}

	std::set<WallID> FindWallsInRelativeRegion(const LocalFrame& frame, const Region& region) {
		GlobalPosition min(frame.origin);
		GlobalPosition max(frame.origin);

		min.pos += region.getMin();
		max.pos += region.getMax();

		min = DoAllRoomChanges(min);
		max = DoAllRoomChanges(max);

		return FindWallsInRegion(min, max);
	}

	std::set<LineID> FindLinesInRegion(const GlobalPosition& from, const GlobalPosition& to) {
		std::set<LineID> result;

		IntVector roomOffset = GetMinOffsetBetweenRooms(from.room, to.room);
		IntVector range = GetMinDistanceBetween(from, to);

		const Region validRange = Region(IntVector::zero(), range);

		for (int rx = 0; rx <= roomOffset.x; rx++) {
			for (int ry = 0; ry <= roomOffset.y; ry++) {
				RoomPosition room((rx + from.room.rx) % 20, (ry + from.room.ry) % 20);
				RoomData& roomData = GetRoomData(room);

				IntVector roomDistance(rx * 320, ry * 240);

				for (int l = 0; l < roomData.lines.size(); l++) {
					GravityLine& line = roomData.lines.at(l);

					IntVector minCorner = line.min;
					IntVector maxCorner = line.max;

					minCorner += roomDistance - from.pos;
					maxCorner += roomDistance - from.pos;

					const Region lineRegion = Region(minCorner, maxCorner);

					if (validRange.intersects(lineRegion)) {
						result.emplace(room, l);
					}
				}
			}
		}

		return result;
	}
	std::set<LineID> FindLinesInRelativeRegion(const LocalFrame& frame, const Region& region) {
		GlobalPosition min(frame.origin);
		GlobalPosition max(frame.origin);

		min.pos += region.getMin();
		max.pos += region.getMax();

		min = DoAllRoomChanges(min);
		max = DoAllRoomChanges(max);

		return FindLinesInRegion(min, max);
	}

	IntVector Corner::GetPrimaryDir(bool inverseGravity) const {
		switch (type) {
			default:
				VVV_exit(-1);
				return IntVector::zero();
			case TopLeft:
				if (inverseGravity) {
					return IntVector(0, -1);
				} else {
					return IntVector(-1, 0);
				}
			case TopRight:
				if (inverseGravity) {
					return IntVector(0, -1);
				} else {
					return IntVector(1, 0);
				}
			case BottomRight:
				if (inverseGravity) {
					return IntVector(1, 0);
				} else {
					return IntVector(0, 1);
				}
			case BottomLeft:
				if (inverseGravity) {
					return IntVector(-1, 0);
				} else {
					return IntVector(0, 1);
				}
		}
	}
	IntVector Corner::GetSecondaryDir(bool inverseGravity) const {
		switch (type) {
			default:
				VVV_exit(-1);
				return IntVector::zero();
			case TopLeft:
				if (inverseGravity) {
					return IntVector(1, 0);
				} else {
					return IntVector(0, 1);
				}
			case TopRight:
				if (inverseGravity) {
					return IntVector(-1, 0);
				} else {
					return IntVector(0, 1);
				}
			case BottomRight:
				if (inverseGravity) {
					return IntVector(0, -1);
				} else {
					return IntVector(-1, 0);
				}
			case BottomLeft:
				if (inverseGravity) {
					return IntVector(0, -1);
				} else {
					return IntVector(1, 0);
				}
		}
	}
	void Corner::GetSpeedRange(FloatInterval& vx, FloatInterval& vy, bool goingUp) const {
		IntVector secondaryDir = GetSecondaryDir(goingUp);
		Exceptions::assert((secondaryDir.x == 0) != (secondaryDir.y == 0));
		if (secondaryDir.x == 0) {
			vx.make_top().intersect(FULL_X_SPEED_RANGE);
			if (secondaryDir.y > 0) {
				vy.make_top().intersect(POS_Y_SPEED_RANGE);
				return;
			} else if (secondaryDir.y < 0) {
				vy.make_top().intersect(NEG_Y_SPEED_RANGE);
				return;
			}
		} else if (secondaryDir.y == 0) {
			vy.make_top().intersect(FULL_Y_SPEED_RANGE);
			if (secondaryDir.x > 0) {
				vx.make_top().intersect(POS_X_SPEED_RANGE);
				return;
			} else if (secondaryDir.x < 0) {
				vx.make_top().intersect(NEG_X_SPEED_RANGE);
				return;
			}
		}

		Exceptions::unreachable();
	}

	// The region to or from which connections can be made
	Region Corner::GetConnectingRegion(bool goingUp) const {
		IntVector primaryDir = GetPrimaryDir(goingUp);
		IntVector primaryGap(horizontalGap, verticalGap);
		primaryGap *= primaryDir;

		// 1 pixel past the corner to 6/10 pixels past the corner
		IntVector secondaryDirMin = GetSecondaryDir(goingUp);
		IntVector secondaryDirMax = secondaryDirMin * IntVector(MAX_X_SPEED, MAX_Y_SPEED);

		return Region(pos + secondaryDirMin, primaryGap + pos + secondaryDirMax);
	}
	Region Corner::GetRegionBefore(bool goingUp) const {
		IntVector primaryDir = GetPrimaryDir(goingUp);
		IntVector secondaryDir = GetSecondaryDir(goingUp);
		IntVector overallDir = primaryDir + secondaryDir;

		IntVector boundingPos = pos - primaryDir;

		IntInterval x_interval = IntInterval::negative() * overallDir.x + boundingPos.x;
		IntInterval y_interval = IntInterval::negative() * overallDir.y + boundingPos.y;

		return Region(x_interval, y_interval);
	}
	Region Corner::GetIntermediateRegion(void) const {
		IntVector perpDir = GetPrimaryDir(true) + GetPrimaryDir(false);

		IntInterval x_interval = IntInterval::positive() * perpDir.x + pos.x;
		IntInterval y_interval = IntInterval::positive() * perpDir.y + pos.y;

		return Region(x_interval, y_interval);
	}
	Region Corner::GetRegionAfter(bool goingUp) const {
		IntVector primaryDir = GetPrimaryDir(goingUp);
		IntVector secondaryDir = GetSecondaryDir(goingUp);
		IntVector overallDir = primaryDir + secondaryDir;

		IntVector boundingPos = pos + secondaryDir;

		IntInterval x_interval = IntInterval::positive() * overallDir.x + boundingPos.x;
		IntInterval y_interval = IntInterval::positive() * overallDir.y + boundingPos.y;

		return Region(x_interval, y_interval);
	}

	Region Corner::GetValidRegion(bool goingUp) const {
		IntVector gap(horizontalGap - 1, verticalGap - 1);
		IntVector negativeGap(horizontalNegativeGap - 1, verticalNegativeGap - 1);

		IntVector primaryDir = GetPrimaryDir(goingUp);
		IntVector secondaryDir = GetSecondaryDir(goingUp);

		IntVector beforeCorner = pos - secondaryDir * gap;
		IntVector afterCorner = pos + primaryDir * gap + secondaryDir * negativeGap;

		return Region(beforeCorner, afterCorner);
	}

	Region RoomWall::GetConnectingRegion(void) const {
		switch (type) {
			default:
				return Region::bottom();
			case Ceiling:
			case Floor:
				return Region(IntInterval(min + 1, max - 1), IntInterval(plane));
			case LeftWall:
			case RightWall:
				return Region(IntInterval(plane), IntInterval(min + 1, max - 1));
		}
	}

	Region RoomWall::GetBoundedRegion(void) const {
		switch (type) {
			default:
				return Region::bottom();
			case Ceiling:
				return Region::fromYLowerBound(plane);
			case Floor:
				return Region::fromYUpperBound(plane);
			case LeftWall:
				return Region::fromXLowerBound(plane);
			case RightWall:
				return Region::fromXUpperBound(plane);
		}
	}
	Region RoomWall::GetBlockedRegion(void) const {
		switch (type) {
			default:
				return Region::bottom();
			case Ceiling:
				return Region(IntInterval(min + 1, max - 1), IntInterval::fromUpperBound(plane - 1));
			case Floor:
				return Region(IntInterval(min + 1, max - 1), IntInterval::fromLowerBound(plane + 1));
			case LeftWall:
				return Region(IntInterval::fromUpperBound(plane - 1), IntInterval(min + 1, max - 1));
			case RightWall:
				return Region(IntInterval::fromLowerBound(plane + 1), IntInterval(min + 1, max - 1));
		}
	}

	Region GravityLine::GetConnectingRegion(void) const {
		return Region(min, max);
	}

	Region GetConnectingRegionInLocalFrame(GenericID el_id, const LocalFrame& frame) {
		if (!el_id.is_valid()) {
			return Region::bottom();
		}

		if (el_id.isCorner()) {
			CornerID& c_id = el_id.unwrapCorner();
			return GetCornerInLocalFrame(c_id, frame).GetConnectingRegion(c_id.goingUp);
		} else if (el_id.isWall()) {
			return GetWallInLocalFrame(el_id.unwrapWall(), frame).GetConnectingRegion();
		} else if (el_id.isLine()) {
			return GetGravityLineInLocalFrame(el_id.unwrapLine(), frame).GetConnectingRegion();
		} else {
			Exceptions::unimplemented();
		}
	}
	Region GetBeforeConnectionRegionInLocalFrame(GenericID el_id, const LocalFrame& frame) {
		if (!el_id.is_valid()) {
			return Region::bottom();
		}

		if (el_id.isCorner()) {
			CornerID& c_id = el_id.unwrapCorner();
			Corner& c = GetCornerInLocalFrame(c_id, frame);
			return c.GetRegionBefore(c_id.goingUp).join(c.GetIntermediateRegion());
		} else if (el_id.isWall()) {
			return GetWallInLocalFrame(el_id.unwrapWall(), frame).GetBoundedRegion();
		} else if (el_id.isLine()) {
			return Region::top();
		} else {
			Exceptions::unimplemented();
		}
	}
	Region GetAfterConnectionRegionInLocalFrame(GenericID el_id, const LocalFrame& frame) {
		if (!el_id.is_valid()) {
			return Region::bottom();
		}

		if (el_id.isCorner()) {
			CornerID& c_id = el_id.unwrapCorner();
			Corner& c = GetCornerInLocalFrame(c_id, frame);
			return c.GetRegionAfter(c_id.goingUp);
		} else if (el_id.isWall()) {
			return GetWallInLocalFrame(el_id.unwrapWall(), frame).GetBoundedRegion();
		} else if (el_id.isLine()) {
			return Region::top();
		} else {
			Exceptions::unimplemented();
		}
	}

	// Iteratively explore connecting regions
	std::vector<FullConnection> FindCornerConnections(CornerID c_id, bool goingUp) {
		RoomPosition cornerRoom = c_id.room;
		const LocalFrame& frame = LocalFrame(GlobalPosition(cornerRoom, IntVector::zero()), false, false);

		Corner corner = GetCornerInLocalFrame(c_id, frame);

		IntVector gap(corner.horizontalGap, corner.verticalGap);
		IntVector negativeGap(corner.horizontalNegativeGap, corner.verticalNegativeGap);

		IntVector primaryDir = corner.GetPrimaryDir(goingUp);
		IntVector secondaryDir = corner.GetSecondaryDir(goingUp);

		IntVector beforeCorner = corner.pos - secondaryDir * gap;
		IntVector afterCorner = corner.pos + primaryDir * gap + secondaryDir * negativeGap;
		
		Region validRegion(beforeCorner, afterCorner);

		std::set<CornerID> corners;
		{
			std::set<CornerID> tmp_corners = FindCornersInRelativeRegion(frame, validRegion);
			for (std::set<CornerID>::iterator it = tmp_corners.begin(); it != tmp_corners.end(); it++) {
				Corner& corner = GetCornerInLocalFrame(*it, frame);
				if (!validRegion.contains(corner.pos)) {
					continue;
				}
				corners.insert(*it);
			}
		}
		std::set<WallID> surfaces;
		{
			std::set<WallID> walls = FindWallsInRelativeRegion(frame, validRegion);
			for (std::set<WallID>::iterator it = walls.begin(); it != walls.end(); it++) {
				RoomWall& wall = GetWallInLocalFrame(*it, frame);
				if (wall.type != Ceiling && wall.type != Floor) {
					continue;
				}
				if (!wall.walkable) {
					continue;
				}
				if (!wall.GetConnectingRegion().intersects(validRegion)) {
					continue;
				}
				surfaces.insert(*it);
			}
		}
		std::set<LineID> lines;
		{
			std::set<LineID> tmp_lines = FindLinesInRelativeRegion(frame, validRegion);
			for (std::set<LineID>::iterator it = tmp_lines.begin(); it != tmp_lines.end(); it++) {
				GravityLine& line = GetGravityLineInLocalFrame(*it, frame);
				if (!line.GetConnectingRegion().intersects(validRegion)) {
					continue;
				}
				lines.insert(*it);
			}
		}

		std::vector<FullConnection> results;

		for (int inverseGravity = 0; inverseGravity < 2; inverseGravity++) {
			int secondary_y = corner.GetSecondaryDir(goingUp).y;
			if (inverseGravity && secondary_y > 0) {
				continue;
			} else if (!inverseGravity && secondary_y < 0) {
				continue;
			}

			FullConnection history;
			history.fromCorner.corner = c_id;
			history.fromCorner.goingUp = goingUp;
			history.fromCorner.inverseGravity = inverseGravity > 0;
			history.fromCorner.pos = corner.GetConnectingRegion(goingUp);
			history.fromCorner.vx = FloatInterval(-MAX_X_SPEED, MAX_X_SPEED);
			history.fromCorner.vy = (inverseGravity > 0) ? FloatInterval(-MAX_Y_SPEED, 0) : FloatInterval(0, MAX_Y_SPEED);
			history.intermediate_surfaces.clear();

			std::vector<FullConnection> this_result = RecursiveSurfaceConnections(frame, history, surfaces, corners);
			for (std::vector<FullConnection>::iterator it = this_result.begin(); it != this_result.end(); it++) {
				results.push_back(*it);
			}
		}

		return results;
	}

	std::vector<FullConnection> RecursiveSurfaceConnections(const LocalFrame& frame, const FullConnection& history, const std::set<WallID>& surfaces, const std::set<CornerID>& corners) {
		const CornerConnection& src = history.fromCorner;
		const Corner& srcCorner = GetCornerInLocalFrame(src.corner, frame);

		bool is_first_conn = history.intermediate_surfaces.empty();

		const Region& initialPos = src.pos;

		int x_max_lower_bound, x_min_upper_bound;
		int y_max_lower_bound, y_min_upper_bound;
		
		x_max_lower_bound = initialPos.x.min;
		x_min_upper_bound = initialPos.x.max;
		y_max_lower_bound = initialPos.y.min;
		y_min_upper_bound = initialPos.y.max;

		Region validRegion = srcCorner.GetValidRegion(src.goingUp);
		if (is_first_conn) {
			// Soft corner bounds due to directionality constraint
			validRegion.intersect(srcCorner.GetRegionAfter(src.goingUp));
		} else {
			// Further restrict connections based on which surfaces we've already visited
			for (std::vector<SurfaceConnection>::const_iterator it2 = history.intermediate_surfaces.begin(); it2 != history.intermediate_surfaces.end(); ++it2) {
				const SurfaceConnection& surfaceConn = *it2;
				const RoomWall& surface = GetWallInLocalFrame(surfaceConn.surface, frame);

				// Only connect to things above or on the same level as this surface
				validRegion.intersect(surface.GetBoundedRegion());

				const Region& intermediatePos = surfaceConn.pos;
				x_max_lower_bound = SDL_max(intermediatePos.x.min, x_max_lower_bound);
				x_min_upper_bound = SDL_min(intermediatePos.x.max, x_min_upper_bound);
				y_max_lower_bound = SDL_max(intermediatePos.y.min, y_max_lower_bound);
				y_min_upper_bound = SDL_min(intermediatePos.y.max, y_min_upper_bound);
			}
		}

		std::vector<FullConnection> results;

		bool can_flip = !is_first_conn;
		Region currentPos = is_first_conn ? src.pos : history.intermediate_surfaces.back().pos;
		FloatInterval v_x = is_first_conn ? src.vx : history.intermediate_surfaces.back().vx;
		FloatInterval v_y = is_first_conn ? src.vy : history.intermediate_surfaces.back().vy;
		bool inverseGravity = is_first_conn ? src.inverseGravity : history.intermediate_surfaces.back().inverseGravity;

		if (can_flip) {
			// First, find what we can connect to without flipping
			// TODO: handle this case
			// TODO: also handle edge flips / ghost pixels

			// What proportion of the current surface do we already cover?
			const SurfaceConnection& lastConn = history.intermediate_surfaces.back();
			const RoomWall& surface = GetWallInLocalFrame(lastConn.surface, frame);
			Region leftPart(surface.GetConnectingRegion());
			leftPart.difference(Region(currentPos).removeXUpperBound());
			Region rightPart(surface.GetConnectingRegion());
			rightPart.difference(Region(currentPos).removeXLowerBound());

			if (leftPart.is_bottom()) {
				// Left corner is available to edge flip or drop off

				// TODO
			} else {
				// Add walking connection to left part
				// TODO: reduce by walking connectivity? doesn't seem necessary (yet)
				// Restrict position after to strictly new positions
				for (std::vector<SurfaceConnection>::const_iterator it2 = history.intermediate_surfaces.begin(); it2 != history.intermediate_surfaces.end(); it2++) {
					const SurfaceConnection& prevConn = *it2;
					if (prevConn.surface == lastConn.surface) {
						leftPart.difference(prevConn.pos);
					}
				}
				if (!leftPart.is_bottom()) {
					SurfaceConnection leftWalkingConn;
					leftWalkingConn.inverseGravity = inverseGravity;
					leftWalkingConn.pos = leftPart;
					leftWalkingConn.surface = lastConn.surface;
					leftWalkingConn.vx = FloatInterval(-MAX_X_SPEED, MAX_X_SPEED);
					leftWalkingConn.vy = inverseGravity ? 0.0f : FloatInterval(0, 0.75f);

					FullConnection partialResult(history);
					partialResult.intermediate_surfaces.push_back(leftWalkingConn);
					std::vector<FullConnection> recursive_results = RecursiveSurfaceConnections(frame, partialResult, surfaces, corners);
					for (std::vector<FullConnection>::iterator it2 = recursive_results.begin(); it2 != recursive_results.end(); it2++) {
						results.push_back(*it2);
					}
				}
			}
			if (rightPart.is_bottom()) {
				// Right corner is available to edge flip

				// TODO
			} else {
				// Add walking connection to right part
				// TODO: reduce by walking connectivity? doesn't seem necessary (yet)
				// Restrict position after to strictly new positions
				for (std::vector<SurfaceConnection>::const_iterator it2 = history.intermediate_surfaces.begin(); it2 != history.intermediate_surfaces.end(); it2++) {
					const SurfaceConnection& prevConn = *it2;
					if (prevConn.surface == lastConn.surface) {
						rightPart.difference(prevConn.pos);
					}
				}
				if (!rightPart.is_bottom()) {
					SurfaceConnection rightWalkingConn;
					rightWalkingConn.inverseGravity = inverseGravity;
					rightWalkingConn.pos = rightPart;
					rightWalkingConn.surface = lastConn.surface;
					rightWalkingConn.vx = FloatInterval(-MAX_X_SPEED, MAX_X_SPEED);
					rightWalkingConn.vy = inverseGravity ? 0.0f : FloatInterval(0, 0.75f);

					FullConnection partialResult(history);
					partialResult.intermediate_surfaces.push_back(rightWalkingConn);
					std::vector<FullConnection> recursive_results = RecursiveSurfaceConnections(frame, partialResult, surfaces, corners);
					for (std::vector<FullConnection>::iterator it2 = recursive_results.begin(); it2 != recursive_results.end(); it2++) {
						results.push_back(*it2);
					}
				}
			}

			// Now, execute the flip and continue
			inverseGravity = !inverseGravity;
			v_y = inverseGravity ? FloatInterval(-6.75f) : FloatInterval(6.75f);
		}

		// Iterate through all corners in valid region
		for (std::set<CornerID>::const_iterator it = corners.begin(); it != corners.end(); it++) {
			const Corner& c = GetCornerInLocalFrame(*it, frame);

			if (!validRegion.contains(c.pos)) {
				continue;
			}

			// TODO: eliminate this for loop, shouldn't be necessary
			for (int goingUp = 0; goingUp < 2; goingUp++) {
				// We aren't currently before the corner
				// TODO: this isn't quite correct, i think. need to use min_lower bound stuff
				if (!c.GetRegionBefore(goingUp > 0).intersects(currentPos)) {
					continue;
				}
				// Don't connect to self in these cases
				if (history.fromCorner.corner == *it && (is_first_conn || history.fromCorner.goingUp == (goingUp > 0))) {
					continue;
				}
				int requiredGravity = c.GetSecondaryDir(goingUp).y;
				if (requiredGravity != 0) {
					if (inverseGravity && requiredGravity == 1) {
						continue;
					} else if (!inverseGravity && requiredGravity == -1) {
						continue;
					}
				}

				Region posBefore(currentPos);
				posBefore.intersect(c.GetRegionBefore(goingUp > 0));
				Region posAfter = c.GetConnectingRegion(goingUp > 0);

				ReduceByFliplessConnectivity(posBefore, posAfter, v_x, v_y, inverseGravity, false);

				// No connection possible
				if (posBefore.is_bottom() || posAfter.is_bottom()) {
					continue;
				}

				CornerConnection finalConn;
				finalConn.corner = *it;
				finalConn.goingUp = goingUp > 0;
				finalConn.inverseGravity = inverseGravity;
				finalConn.pos = posAfter;
				// TODO: better vx / vy estimates
				finalConn.vx = FloatInterval(-MAX_X_SPEED, MAX_X_SPEED);
				finalConn.vy = FloatInterval(-MAX_Y_SPEED, MAX_Y_SPEED);

				FullConnection fullResult(history);
				if (is_first_conn) {
					fullResult.fromCorner.pos = posBefore;
				} else {
					fullResult.intermediate_surfaces.back().pos = posBefore;
				}
				fullResult.toCorner = finalConn;

				// Propagate restrictions backwards
				Region lastPos = fullResult.toCorner.pos;
				bool to_surface = false;
				bool lastGravity = fullResult.toCorner.inverseGravity;
				for (std::vector<SurfaceConnection>::reverse_iterator it = fullResult.intermediate_surfaces.rbegin(); it != fullResult.intermediate_surfaces.rend(); it++) {
					SurfaceConnection& s = *it;
					if (!to_surface || lastGravity != s.inverseGravity) {
						float svy = lastGravity ? -6.75f : 6.75f;
						ReduceByFliplessConnectivity(s.pos, lastPos, s.vx, svy, lastGravity, to_surface);
					}
					to_surface = true;
					lastPos = s.pos;
					lastGravity = s.inverseGravity;
				}
				ReduceByFliplessConnectivity(fullResult.fromCorner.pos, lastPos, fullResult.fromCorner.vx, fullResult.fromCorner.vy, lastGravity, true);

				results.push_back(fullResult);
			}
		}

		// Iterate through all surfaces in valid region
		// For each one that we can connect to, make a recursive call
		for (std::set<WallID>::const_iterator it = surfaces.begin(); it != surfaces.end(); it++) {
			const RoomWall& s = GetWallInLocalFrame(*it, frame);

			if (inverseGravity && s.type != Ceiling) {
				continue;
			} else if (!inverseGravity && s.type != Floor) {
				continue;
			}

			Region connRegion = s.GetConnectingRegion();
			connRegion.intersect(validRegion);
			// No connection is possible (or no further connections are possible
			if (connRegion.is_bottom() || !s.GetBoundedRegion().intersects(validRegion)) {
				continue;
			}

			Region posBefore(currentPos);
			Region posAfter = connRegion;

			ReduceByFliplessConnectivity(posBefore, posAfter, v_x, v_y, inverseGravity, true);

			// No connectivity
			if (posBefore.is_bottom() || posAfter.is_bottom()) {
				continue;
			}

			// Restrict position after to strictly new positions
			// TODO: split based on left/right here? or actually, combine intervals per surface or something?
			// or iterate over free intervals per surface
			// because revisiting same surface can have weird cases
			for (std::vector<SurfaceConnection>::const_iterator it2 = history.intermediate_surfaces.begin(); it2 != history.intermediate_surfaces.end(); it2++) {
				const SurfaceConnection& prevConn = *it2;
				if (prevConn.surface == *it) {
					posAfter.difference(prevConn.pos);
				}
			}

			// No new reachable positions
			if (posAfter.is_bottom()) {
				continue;
			}

			SurfaceConnection surfConn;
			surfConn.surface = *it;
			surfConn.pos = posAfter;
			surfConn.vx = FloatInterval(-MAX_X_SPEED, MAX_X_SPEED);
			surfConn.vy = inverseGravity ? FloatInterval(-MAX_Y_SPEED, 0) : FloatInterval(0, MAX_Y_SPEED);
			surfConn.inverseGravity = inverseGravity;

			FullConnection partialResult(history);
			if (is_first_conn) {
				partialResult.fromCorner.pos = posBefore;
			} else {
				partialResult.intermediate_surfaces.back().pos = posBefore;
			}
			partialResult.intermediate_surfaces.push_back(surfConn);

			// Propagate restrictions backwards
			Region lastPos = partialResult.intermediate_surfaces.back().pos;
			bool is_first = true;
			bool lastGravity = partialResult.intermediate_surfaces.back().inverseGravity;
			for (std::vector<SurfaceConnection>::reverse_iterator it = partialResult.intermediate_surfaces.rbegin(); it != partialResult.intermediate_surfaces.rend(); it++) {
				SurfaceConnection& s = *it;
				if (!is_first) {
					if (lastGravity != s.inverseGravity) {
						ReduceByFliplessConnectivity(s.pos, lastPos, s.vx, s.vy, lastGravity, true);
					}
				} else {
					is_first = false;
				}
				lastPos = s.pos;
				lastGravity = s.inverseGravity;
			}
			ReduceByFliplessConnectivity(partialResult.fromCorner.pos, lastPos, partialResult.fromCorner.vx, partialResult.fromCorner.vy, lastGravity, true);

			std::vector<FullConnection> recursive_results = RecursiveSurfaceConnections(frame, partialResult, surfaces, corners);
			for (std::vector<FullConnection>::iterator it2 = recursive_results.begin(); it2 != recursive_results.end(); it2++) {
				results.push_back(*it2);
			}
		}

		return results;
	}

	std::vector<WaypointPath> NewFindCornerConnections(CornerID c_id) {
		RoomPosition cornerRoom = c_id.room;
		const LocalFrame& frame = LocalFrame(GlobalPosition(cornerRoom, IntVector::zero()), false, false);

		Corner corner = GetCornerInLocalFrame(c_id, frame);

		IntVector gap(corner.horizontalGap, corner.verticalGap);
		IntVector negativeGap(corner.horizontalNegativeGap, corner.verticalNegativeGap);

		IntVector primaryDir = corner.GetPrimaryDir(c_id.goingUp);
		IntVector secondaryDir = corner.GetSecondaryDir(c_id.goingUp);

		IntVector beforeCorner = corner.pos - secondaryDir * gap;
		IntVector afterCorner = corner.pos + primaryDir * gap + secondaryDir * negativeGap;

		Region validRegion(beforeCorner, afterCorner);

		std::set<GenericID> elements;
		// Corners
		{
			std::set<CornerID> tmp_corners = FindCornersInRelativeRegion(frame, validRegion);
			for (std::set<CornerID>::iterator it = tmp_corners.begin(); it != tmp_corners.end(); it++) {
				Corner& corner = GetCornerInLocalFrame(*it, frame);
				if (!validRegion.contains(corner.pos)) {
					continue;
				}
				elements.emplace(*it);
			}
		}
		// Walls
		{
			std::set<WallID> walls = FindWallsInRelativeRegion(frame, validRegion);
			for (std::set<WallID>::iterator it = walls.begin(); it != walls.end(); it++) {
				RoomWall& wall = GetWallInLocalFrame(*it, frame);
				if (wall.type != Ceiling && wall.type != Floor) {
					continue;
				}
				if (!wall.walkable) {
					continue;
				}
				if (!wall.GetConnectingRegion().intersects(validRegion)) {
					continue;
				}
				elements.emplace(*it);
			}
		}
		// Lines
		{
			std::set<LineID> tmp_lines = FindLinesInRelativeRegion(frame, validRegion);
			for (std::set<LineID>::iterator it = tmp_lines.begin(); it != tmp_lines.end(); it++) {
				GravityLine& line = GetGravityLineInLocalFrame(*it, frame);
				if (!line.GetConnectingRegion().intersects(validRegion)) {
					continue;
				}
				elements.emplace(*it);
			}
		}

		std::vector<WaypointPath> results;
		for (int inverseGravity = 0; inverseGravity < 2; inverseGravity++) {
			int secondary_y = corner.GetSecondaryDir(c_id.goingUp).y;
			if (inverseGravity && secondary_y > 0) {
				continue;
			} else if (!inverseGravity && secondary_y < 0) {
				continue;
			}

			CornerWaypoint source;

			source.corner_id = c_id;
			source.playerState.inverseGravity = inverseGravity;
			source.playerState.pos = corner.GetConnectingRegion(c_id.goingUp);
			corner.GetSpeedRange(source.playerState.vx, source.playerState.vy, c_id.goingUp);
			WaypointPath history(source);

			std::vector<WaypointPath> this_result = NewRecursiveSurfaceConnections(frame, history, elements);
			for (std::vector<WaypointPath>::iterator it = this_result.begin(); it != this_result.end(); it++) {
				if ((*it).target.playerState.is_bottom()) {
					continue;
				}
				results.push_back(*it);
			}
		}

		return results;
	}

	std::vector<WaypointPath> RevampedFindCornerConnections(CornerID c_id) {
		RoomPosition cornerRoom = c_id.room;
		const LocalFrame& frame = LocalFrame(GlobalPosition(cornerRoom, IntVector::zero()), false, false);

		Corner corner = GetCornerInLocalFrame(c_id, frame);

		IntVector gap(corner.horizontalGap, corner.verticalGap);
		IntVector negativeGap(corner.horizontalNegativeGap, corner.verticalNegativeGap);

		IntVector primaryDir = corner.GetPrimaryDir(c_id.goingUp);
		IntVector secondaryDir = corner.GetSecondaryDir(c_id.goingUp);

		IntVector beforeCorner = corner.pos - secondaryDir * gap;
		IntVector afterCorner = corner.pos + primaryDir * gap + secondaryDir * negativeGap;

		Region validRegion(beforeCorner, afterCorner);

		std::set<GenericID> elements;
		// Corners
		{
			std::set<CornerID> tmp_corners = FindCornersInRelativeRegion(frame, validRegion);
			for (std::set<CornerID>::iterator it = tmp_corners.begin(); it != tmp_corners.end(); it++) {
				Corner& corner = GetCornerInLocalFrame(*it, frame);
				if (!validRegion.contains(corner.pos)) {
					continue;
				}
				elements.emplace(*it);
			}
		}
		// Walls
		{
			std::set<WallID> walls = FindWallsInRelativeRegion(frame, validRegion);
			for (std::set<WallID>::iterator it = walls.begin(); it != walls.end(); it++) {
				RoomWall& wall = GetWallInLocalFrame(*it, frame);
				if (wall.type != Ceiling && wall.type != Floor) {
					continue;
				}
				if (!wall.walkable) {
					continue;
				}
				if (!wall.GetConnectingRegion().intersects(validRegion)) {
					continue;
				}
				elements.emplace(*it);
			}
		}
		// Lines
		{
			std::set<LineID> tmp_lines = FindLinesInRelativeRegion(frame, validRegion);
			for (std::set<LineID>::iterator it = tmp_lines.begin(); it != tmp_lines.end(); it++) {
				GravityLine& line = GetGravityLineInLocalFrame(*it, frame);
				if (!line.GetConnectingRegion().intersects(validRegion)) {
					continue;
				}
				elements.emplace(*it);
			}
		}

		std::vector<WaypointPath> results;
		for (int inverseGravity = 0; inverseGravity < 2; inverseGravity++) {
			int secondary_y = corner.GetSecondaryDir(c_id.goingUp).y;
			if (inverseGravity && secondary_y > 0) {
				continue;
			}
			else if (!inverseGravity && secondary_y < 0) {
				continue;
			}

			CornerWaypoint source;

			source.corner_id = c_id;
			source.playerState.inverseGravity = inverseGravity;
			source.playerState.pos = corner.GetConnectingRegion(c_id.goingUp);
			corner.GetSpeedRange(source.playerState.vx, source.playerState.vy, c_id.goingUp);
			WaypointPath history(source);

			std::vector<WaypointPath> this_result = RevampedRecursiveSurfaceConnections(frame, history, elements);
			for (std::vector<WaypointPath>::iterator it = this_result.begin(); it != this_result.end(); it++) {
				if ((*it).target.playerState.is_bottom()) {
					continue;
				}
				results.push_back(*it);
			}
		}

		return results;
	}

	std::vector<WaypointPath> RevampedRecursiveSurfaceConnections(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements) {
		std::vector<WaypointPath> results;
		if (history.getLastPlayerStateConst().is_bottom()) {
			return results;
		} else if (!history.is_partial()) {
			results.push_back(history);
			return results;
		}

		std::vector<WaypointPath> new_paths;

		const Region validConnectionRegion = RevampedGetValidConnectionRegion(frame, history);
		std::vector<PlayerStateRange> outgoingPrevStates = RevampedGetOutgoingConnectionStates(frame, history);
		std::vector<ElementRegion> nextConnections = RevampedGetNextPossibleConnections(frame, history, elements);

		// Create all new possible paths (some will inevitably be invalid)
		for (std::vector<PlayerStateRange>::const_iterator prev_state_it = outgoingPrevStates.cbegin(); prev_state_it != outgoingPrevStates.cend(); prev_state_it++) {
			const PlayerStateRange& prevStateOut = *prev_state_it;
			Exceptions::assert(!prevStateOut.is_bottom());
			for (std::vector<ElementRegion>::const_iterator next_conn_it = nextConnections.cbegin(); next_conn_it != nextConnections.cend(); next_conn_it++) {
				const ElementRegion& nextConn = *next_conn_it;
				Exceptions::assert(nextConn.element_id.is_valid() && !nextConn.region.is_bottom());

				PlayerStateRange newPrevState(prevStateOut);
				PlayerStateRange nextStateIn = PlayerStateRange::top();
				nextStateIn.inverseGravity = newPrevState.inverseGravity;
				nextStateIn.pos.intersect(nextConn.region);

				// TODO: better reduction
				ReduceByFliplessConnectivity(newPrevState.pos, nextStateIn.pos, newPrevState.vx, newPrevState.vy, newPrevState.inverseGravity, nextConn.element_id.isWall());
				
				if (newPrevState.is_bottom() || nextStateIn.is_bottom()) {
					continue;
				}

				WaypointPath newPath(history);
				if (newPath.waypoints.empty()) {
					newPath.source.playerState = newPrevState;
				} else {
					newPath.waypoints.back().playerStateOut = newPrevState;
				}
				if (nextConn.element_id.isCorner()) {
					newPath.target.corner_id = nextConn.element_id.unwrapCorner();
					newPath.target.playerState = nextStateIn;
				} else {
					GenericWaypoint newWaypoint;
					newWaypoint.id = nextConn.element_id;
					newWaypoint.playerState = nextStateIn;
					newWaypoint.playerStateOut = PlayerStateRange::bottom();

					newPath.waypoints.push_back(newWaypoint);
				}

				new_paths.push_back(newPath);
			}
		}

		// Reduce paths by connectivity, make recursive calls to complete them if necessary
		for (std::vector<WaypointPath>::iterator path_it = new_paths.begin(); path_it != new_paths.end(); path_it++) {
			WaypointPath& newPath = *path_it;

			// TODO: reduce the paths by connectivity

			// Add the path to the results
			if (newPath.is_partial()) {
				// Make recursive call to complete the path
				std::vector<WaypointPath> rec_results = RevampedRecursiveSurfaceConnections(frame, newPath, elements);
				results.insert(results.end(), rec_results.begin(), rec_results.end());
			} else {
				results.push_back(newPath);
			}
		}

		return results;
	}

	Region RevampedGetValidConnectionRegion(const LocalFrame& frame, const WaypointPath& history) {
		GenericID last_element_id = history.getLastElementID();

		// Determine valid region for connections based on corner region and previous waypoints
		Region validConnectionRegion = GetCornerInLocalFrame(history.source.corner_id, frame).GetValidRegion(history.source.corner_id.goingUp);
		if (last_element_id.isCorner()) {
			CornerID last_corner_id = last_element_id.unwrapCorner();
			const Corner& last_corner = GetCornerInLocalFrame(last_corner_id, frame);
			validConnectionRegion.intersect(last_corner.GetRegionAfter(last_corner_id.goingUp));
		} else if (last_element_id.isWall()) {
			WallID last_wall_id = last_element_id.unwrapWall();
			Region wallRegion = GetWallInLocalFrame(last_wall_id, frame).GetBoundedRegion();
			validConnectionRegion.intersect(wallRegion);
		} else if (last_element_id.isLine()) {
			// TODO: anything here?
		} else {
			Exceptions::unimplemented();
		}

		return validConnectionRegion;
	}

	std::vector<PlayerStateRange> RevampedGetOutgoingConnectionStates(const LocalFrame& frame, const WaypointPath& history) {
		std::vector<PlayerStateRange> prev_states;

		GenericID last_element_id = history.getLastElementID();
		PlayerStateRange prevState(history.getLastPlayerStateConst());

		// Determine outgoing states from previous waypoint
		if (last_element_id.isCorner()) {
			// Corner simply propagates state
			prev_states.push_back(prevState);
		} else if (last_element_id.isLine()) {
			// Gravity line forces a flip
			DoGravityLineFlip(prevState);
			prev_states.push_back(prevState);
		} else if (last_element_id.isWall()) {
			RoomWall& surface = GetWallInLocalFrame(last_element_id.unwrapWall(), frame);
			const Region walkingRegion = surface.GetConnectingRegion();
			// A few cases here:
			// 1. walk to a position and flip (without walking off the platform)
			PlayerStateRange walkThenFlip(prevState);
			walkThenFlip.pos = Region(walkingRegion);
			walkThenFlip.vx = FULL_X_SPEED_RANGE;
			walkThenFlip.vy = PLATFORM_Y_SPEED_RANGE_FOR_GRAVITY(prevState.inverseGravity);
			DoFlip(walkThenFlip);
			prev_states.push_back(walkThenFlip);

			// 2a. walk off the left edge of the platform (if possible)
			if (!surface.minCornerConcave) {
				// The state should be 1 frame before walking off
				// TODO: is this correct?
				PlayerStateRange walkOffLeft(prevState);
				walkOffLeft.inverseGravity = prevState.inverseGravity;
				walkOffLeft.pos = Region(walkingRegion).addXUpperBound(walkingRegion.x.getLowerBound() + MAX_X_SPEED - 1);
				walkOffLeft.vx = NEG_X_SPEED_RANGE;
				walkOffLeft.vy = PLATFORM_Y_SPEED_RANGE_FOR_GRAVITY(prevState.inverseGravity);
				prev_states.push_back(walkOffLeft);
			}
			// 2b. walk off the right edge of the platform (if possible)
			if (!surface.maxCornerConcave) {
				// The state should be 1 frame before walking off
				// TODO: is this correct?
				PlayerStateRange walkOffRight(prevState);
				walkOffRight.inverseGravity = prevState.inverseGravity;
				walkOffRight.pos = Region(walkingRegion).addXLowerBound(walkingRegion.x.getUpperBound() - MAX_X_SPEED + 1);
				walkOffRight.vx = POS_X_SPEED_RANGE;
				walkOffRight.vy = PLATFORM_Y_SPEED_RANGE_FOR_GRAVITY(prevState.inverseGravity);
				prev_states.push_back(walkOffRight);
			}

			// 3. edge flip first frame (left & right)
			// TODO
			// 4. edge flip second frame (left & right)
			// TODO
		} else {
			Exceptions::unimplemented();
		}

		return prev_states;
	}

	std::vector<ElementRegion> RevampedGetCoveredRanges(const LocalFrame& frame, const WaypointPath& history) {
		std::vector<ElementRegion> coveredRanges;

		// Go through the history, covering every region of every platform that definitely could have been reached using the same connections
		PlayerStateRange definitelyReachable(history.source.playerState);
		PlayerStateRange maybeReachable(history.source.playerState);

		Region totalDefinitelyReached(definitelyReachable.pos);
		Region totalMaybeReached(maybeReachable.pos);

		GenericWaypoint& const lastWp = GenericWaypoint(history.source);
		// Iterate over all previous waypoints
		for (std::vector<GenericWaypoint>::const_iterator it = history.waypoints.cbegin(); it != history.waypoints.cend(); it++) {
			const GenericWaypoint& wp = *it;

			bool fromSurface = lastWp.id.isWall();
			bool toSurface = wp.id.isWall();

			const PlayerStateRange& fromStateOut = lastWp.playerStateOut;

			const PlayerStateRange& toStateIn = wp.playerState;
			const PlayerStateRange& toStateOut = wp.playerStateOut;

			Exceptions::assert(fromStateOut.inverseGravity == definitelyReachable.inverseGravity);
			Exceptions::assert(fromStateOut.inverseGravity == maybeReachable.inverseGravity);

			// Create new player states
			PlayerStateRange newDefinitelyReachable = PlayerStateRange::top();
			newDefinitelyReachable.pos.intersect(GetConnectingRegionInLocalFrame(wp.id, frame));
			PlayerStateRange newMaybeReachable(newDefinitelyReachable);

			// Reduce them as much as possible by connectivity
			ReduceByFliplessConnectivity(definitelyReachable.pos, newDefinitelyReachable.pos, definitelyReachable.vx, definitelyReachable.vy, definitelyReachable.inverseGravity, toSurface);
			ReduceByFliplessConnectivity(maybeReachable.pos, newMaybeReachable.pos, maybeReachable.vx, maybeReachable.vy, maybeReachable.inverseGravity, toSurface);

			Exceptions::assert(!newDefinitelyReachable.is_bottom() && !newMaybeReachable.is_bottom());

			coveredRanges.emplace_back(wp.id, newMaybeReachable.pos);

			lastWp = wp;
			totalDefinitelyReached.join(newDefinitelyReachable.pos);
			totalMaybeReached.join(newMaybeReachable.pos);
			definitelyReachable = newDefinitelyReachable;
			maybeReachable = newMaybeReachable;

			// Walking logic and flipping logic (in -> out state)
			if (wp.id.isCorner()) {
				// Do nothing
			} else if (wp.id.isLine()) {
				// Do regular gravity line flip
				DoGravityLineFlip(definitelyReachable);
				DoGravityLineFlip(maybeReachable);
			} else if (wp.id.isWall()) {
				RoomWall& surface = GetWallInLocalFrame(wp.id.unwrapWall(), frame);
				maybeReachable.pos.join(surface.GetConnectingRegion());
				maybeReachable.vx.join(FULL_X_SPEED_RANGE);
				maybeReachable.vy.join(PLATFORM_Y_SPEED_RANGE_FOR_GRAVITY(toStateIn.inverseGravity));
				if (fromStateOut.inverseGravity != toStateIn.inverseGravity) {
					// Flip
					DoFlip(definitelyReachable);
					DoFlip(maybeReachable);
				}
				// TODO: add edgeflips to maybeReachable
			} else {
				// Unimplemented waypoint type
				Exceptions::unimplemented();
			}
		}

		if (!history.is_partial()) {
			const GenericID target_el_id = GenericID(history.target.corner_id);

			const PlayerStateRange& fromStateOut = lastWp.playerStateOut;
			const PlayerStateRange& toState = history.target.playerState;

			Exceptions::assert(fromStateOut.inverseGravity == definitelyReachable.inverseGravity);
			Exceptions::assert(fromStateOut.inverseGravity == maybeReachable.inverseGravity);

			// Create new player states
			PlayerStateRange newDefinitelyReachable = PlayerStateRange::top();
			newDefinitelyReachable.pos.intersect(GetConnectingRegionInLocalFrame(target_el_id, frame));
			PlayerStateRange newMaybeReachable(newDefinitelyReachable);

			// Reduce them as much as possible by connectivity
			ReduceByFliplessConnectivity(definitelyReachable.pos, newDefinitelyReachable.pos, definitelyReachable.vx, definitelyReachable.vy, definitelyReachable.inverseGravity, false);
			ReduceByFliplessConnectivity(maybeReachable.pos, newMaybeReachable.pos, maybeReachable.vx, maybeReachable.vy, maybeReachable.inverseGravity, false);

			Exceptions::assert(!newDefinitelyReachable.is_bottom() && !newMaybeReachable.is_bottom());

			coveredRanges.emplace_back(target_el_id, newMaybeReachable.pos);

			totalDefinitelyReached.join(newDefinitelyReachable.pos);
			totalMaybeReached.join(newMaybeReachable.pos);
			definitelyReachable = newDefinitelyReachable;
			maybeReachable = newMaybeReachable;
		}

		return coveredRanges;
	}

	std::vector<ElementRegion> RevampedGetNextPossibleConnections(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements) {
		std::vector<ElementRegion> coveredRanges = RevampedGetCoveredRanges(frame, history);
		std::vector<ElementRegion> uncoveredRanges;

		bool allow_inbetween_regions = true;

		const Region validConnectionRegion = RevampedGetValidConnectionRegion(frame, history);
		const Region nextConnectionRegion = GetAfterConnectionRegionInLocalFrame(history.getLastElementID(), frame);

		for (std::set<GenericID>::const_iterator el_it = elements.cbegin(); el_it != elements.cend(); el_it++) {
			const GenericID& el_id = *el_it;
			Region elBeforeConnRegion = GetBeforeConnectionRegionInLocalFrame(el_id, frame);
			Region elConnRegion = GetConnectingRegionInLocalFrame(el_id, frame);

			// Collect all ranges for this element
			int firstUncoveredX = INT_MIN;
			std::vector<IntInterval> ordered_disjoint_ranges;
			std::vector<IntInterval> uncovered_disjoint_ranges;
			bool done = false;
			while (!done) {
				int min_lower_bound = INT_MAX;
				IntInterval lowestXInterval = IntInterval::bottom();
				for (std::vector<ElementRegion>::const_iterator range_it = coveredRanges.cbegin(); range_it != coveredRanges.cend(); range_it++) {
					const ElementRegion& range = *range_it;
					if (range.element_id != el_id || range.region.is_bottom()) {
						continue;
					}
					int xLowerBound = range.region.x.getLowerBound();
					if (xLowerBound >= min_lower_bound || xLowerBound < firstUncoveredX) {
						continue;
					}

					min_lower_bound = xLowerBound;
					lowestXInterval = IntInterval(range.region.x);
				}

				// No more intervals found
				if (lowestXInterval.is_bottom()) {
					done = true;
				} else {
					bool anything_changed = true;
					while (anything_changed) {
						anything_changed = false;
						for (std::vector<ElementRegion>::const_iterator range_it = coveredRanges.cbegin(); range_it != coveredRanges.cend(); range_it++) {
							const ElementRegion& range = *range_it;
							if (range.element_id != el_id || range.region.is_bottom()) {
								continue;
							}
							const IntInterval& thisXInterval = range.region.x;
							if (lowestXInterval.contains(thisXInterval) || !lowestXInterval.intersects(thisXInterval)) {
								continue;
							}

							lowestXInterval.join(range.region.x);
							anything_changed = true;
						}
					}
				}

				// Create the next uncovered region
				IntInterval uncoveredRange = lowestXInterval.getIntervalBelow().addLowerBound(firstUncoveredX);
				firstUncoveredX = lowestXInterval.getIntervalAbove().getLowerBound();

				if (uncoveredRange.is_bottom()) {
					// TODO: does this ever happen? probably
					continue;
				} else if (!allow_inbetween_regions && uncoveredRange.is_bounded()) {
					continue;
				}

				// Can the element we're coming from connect to this new region?
				Region uncoveredRegion(uncoveredRange, IntInterval::top());
				uncoveredRegion.intersect(elConnRegion);
				// Special case for corners
				if (el_id.isCorner()) {
					const CornerID& el_corner_id = el_id.unwrapCorner();
					const Corner& el_corner = GetCornerInLocalFrame(el_corner_id, frame);
					if (!validConnectionRegion.contains(el_corner.pos)) {
						continue;
					}
					uncoveredRegion.intersect(nextConnectionRegion);
				} else {
					uncoveredRegion.intersect(validConnectionRegion);
				}

				if (uncoveredRegion.is_bottom()) {
					// TODO: does this ever happen? -> yes
					continue;
				}

				const Region& posBefore = history.getLastPlayerStateConst().pos;
				if (!posBefore.intersects(elBeforeConnRegion)) {
					// TODO: does this ever happen? -> yes
					continue;
				}

				uncoveredRanges.emplace_back(el_id, uncoveredRegion);
			}
		}

		return uncoveredRanges;
	}

	std::vector<WaypointPath> NewRecursiveSurfaceConnections(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements) {
		std::vector<WaypointPath> results;
		
		std::vector<PlayerStateRange> prev_states;

		std::vector<WaypointPath> new_paths;

		GenericID prev_el_id = history.getLastElementID();

		// Determine valid region for connections based on corner region and previous waypoints
		Region validConnectionRegion = GetCornerInLocalFrame(history.source.corner_id, frame).GetValidRegion(history.source.corner_id.goingUp);
		if (prev_el_id.isCorner()) {
			validConnectionRegion.intersect(GetCornerInLocalFrame(prev_el_id.unwrapCorner(), frame).GetRegionAfter(prev_el_id.unwrapCorner().goingUp));
		} else if (prev_el_id.isWall()) {
			validConnectionRegion.intersect(GetWallInLocalFrame(prev_el_id.unwrapWall(), frame).GetBoundedRegion());
		} else if (prev_el_id.isLine()) {
			// TODO: anything here?
		} else {
			Exceptions::unimplemented();
		}

		// Determine outgoing states from previous waypoint
		if (prev_el_id.isCorner()) {
			prev_states.push_back(history.getLastPlayerStateConst());
		} else if (prev_el_id.isWall()) {
			PlayerStateRange prevStateFlipped;
			prevStateFlipped.inverseGravity = history.getLastPlayerStateConst().inverseGravity;
			prevStateFlipped.pos = Region(history.getLastPlayerStateConst().pos);
			prevStateFlipped.vx = FloatInterval(history.getLastPlayerStateConst().vx);
			prevStateFlipped.vy = FloatInterval(history.getLastPlayerStateConst().vy);
			
			const RoomWall& prevSurface = GetWallInLocalFrame(history.waypoints.back().id.unwrapWall(), frame);
			if (prevSurface.walkable) {
				prev_states.push_back(history.getLastPlayerStateConst());
				DoFlip(prevStateFlipped);
				prev_states.push_back(prevStateFlipped);
				// TODO: add corner/edge/ghostpixel flips
			}
		} else if (prev_el_id.isLine()) {
			PlayerStateRange prevStateFlipped;
			prevStateFlipped.inverseGravity = history.getLastPlayerStateConst().inverseGravity;
			prevStateFlipped.pos = Region(history.getLastPlayerStateConst().pos);
			prevStateFlipped.vx = FloatInterval(history.getLastPlayerStateConst().vx);
			prevStateFlipped.vy = FloatInterval(history.getLastPlayerStateConst().vy);
			DoGravityLineFlip(prevStateFlipped);
			prev_states.push_back(prevStateFlipped);
		} else {
			Exceptions::unimplemented();
		}

		// Get the regions of each element that can be connected to
		std::vector<ElementRegion> element_regions = GetUncoveredRanges(frame, history, elements);

		// Create all possible further connections
		for (std::vector<ElementRegion>::const_iterator it = element_regions.cbegin(); it != element_regions.cend(); it++) {
			const ElementRegion& element_region = *it;
			const GenericID& el_id = element_region.element_id;
			const Region& el_region = element_region.region;

			Exceptions::assert(el_id.is_valid());
			// Don't connect to self
			if (el_id == GenericID(history.source.corner_id) || el_region.is_bottom()) {
				continue;
			}

			for (std::vector<PlayerStateRange>::const_iterator it3 = prev_states.cbegin(); it3 != prev_states.cend(); it3++) {
				PlayerStateRange prevState(*it3);
				Region elConnRegion(el_region);

				if (el_id.isWall()) {
					RoomWall& w = GetWallInLocalFrame(el_id.unwrapWall(), frame);
					if (prevState.inverseGravity && w.type != Ceiling) {
						continue;
					} else if (!prevState.inverseGravity && w.type != Floor) {
						continue;
					}

					// Restrict previous state to valid region
					prevState.pos.intersect(w.GetBoundedRegion());
					// Restrict next state to valid region
					elConnRegion.intersect(validConnectionRegion);

					GenericWaypoint nextWp;
					if (el_id == prev_el_id) {
						// Can't have two walking connections in a row!
						if (history.waypoints.size() > 1) {
							if (history.waypoints.at(history.waypoints.size() - 2).id == prev_el_id) {
								continue;
							}
						}

						// Walking connection -> must be adjacent region
						if (elConnRegion.x.intersects(prevState.pos.x)) {
							// There's an overlap due to may/must analysis, so defer previous position to that
							prevState.pos.x.difference(elConnRegion.x);
							// Sanity check
							if (prevState.pos.x.is_bottom()) {
								continue; // TODO: remove this
							}
							Exceptions::assert(!prevState.pos.x.is_bottom());
						} else {
							// Make sure the regions are adjacent
							if ((elConnRegion.x - prevState.pos.x).abs().getLowerBound() > 1) {
								continue;
							}
						}
						// elConnRegion.x.join(prevState.pos.x);

						nextWp.id = el_id;
						nextWp.playerState.inverseGravity = prevState.inverseGravity;
						nextWp.playerState.pos = elConnRegion;
						nextWp.playerState.vy = PLATFORM_Y_SPEED_RANGE_FOR_GRAVITY(prevState.inverseGravity);
						if (prevState.pos.x.max + 1 == elConnRegion.x.min) {
							// Going right
							nextWp.playerState.vx = POS_X_SPEED_RANGE;
						} else if (prevState.pos.x.min - 1 == elConnRegion.x.max) {
							// Going left
							nextWp.playerState.vx = NEG_X_SPEED_RANGE;
						} else {
							continue;
						}

						// TODO: is there any way we could restrict the regions?
					} else {
						// Can't have two non-walking connections in a row!
						// TODO: is this the right place to enforce this???
						if (prev_el_id.isWall() && history.waypoints.size() > 1) {
							GenericID prev_prev_el_id = history.waypoints.at(history.waypoints.size() - 2).id;
							if (prev_prev_el_id.isWall() && prev_prev_el_id != prev_el_id) {
								continue;
							}
						}

						nextWp.id = el_id;
						nextWp.playerState.inverseGravity = prevState.inverseGravity;
						nextWp.playerState.pos = elConnRegion;
						nextWp.playerState.vx = FULL_X_SPEED_RANGE;
						nextWp.playerState.vy = Y_SPEED_RANGE_FOR_GRAVITY(prevState.inverseGravity);

						// Restrict regions by how they connect
						ReduceByFliplessConnectivity(prevState.pos, nextWp.playerState.pos, prevState.vx, prevState.vy, prevState.inverseGravity, true);
						// ReduceRangesByConnectivity(prevState, nextWp.playerState, frame, el_id.unwrapWall());
					}

					// Add this path to list
					WaypointPath newPath(history);
					newPath.getLastPlayerState() = prevState;
					newPath.waypoints.push_back(nextWp);
					new_paths.push_back(newPath);
				} else if (el_id.isLine()) {
					if (el_id == prev_el_id) {
						// Don't let lines connect to themselves (for now)
						// TODO: do we need this? I hope not
						continue;
					}
					GravityLine& line = GetGravityLineInLocalFrame(el_id.unwrapLine(), frame);

					// Must connect to line from outside it (even from outside previous covered segments)
					prevState.pos.difference(line.GetConnectingRegion());
					// Restrict next state to valid region
					elConnRegion.intersect(validConnectionRegion);

					GenericWaypoint nextWp;
					nextWp.id = el_id;
					nextWp.playerState.inverseGravity = prevState.inverseGravity;
					nextWp.playerState.pos = elConnRegion;
					nextWp.playerState.vx = FULL_X_SPEED_RANGE;
					nextWp.playerState.vy = FULL_Y_SPEED_RANGE; // TODO: restrict y velocity more

					// Restrict regions by how they connect
					ReduceByFliplessConnectivity(prevState.pos, nextWp.playerState.pos, prevState.vx, prevState.vy, prevState.inverseGravity, false);
					// ReduceRangesByConnectivity(prevState, nextWp.playerState, frame, WallID::invalid());

					// Add this path to list
					WaypointPath newPath(history);
					newPath.getLastPlayerState() = prevState;
					newPath.waypoints.push_back(nextWp);
					new_paths.push_back(newPath);
				} else if (el_id.isCorner()) {
					const CornerID& c_el_id = el_id.unwrapCorner();
					Corner& c = GetCornerInLocalFrame(c_el_id, frame);
					IntVector secondaryDir = c.GetSecondaryDir(c_el_id.goingUp);

					// TODO: remove this, it stops walls connecting to their own corners
					if (c.pos.y == prevState.pos.y.getLowerBound() && prevState.pos.y.is_exact()) {
						continue;
					}

					int corner_y_dir = secondaryDir.y;
					if (corner_y_dir != 0) {
						bool requiredGravity = corner_y_dir == -1;
						if (prevState.inverseGravity != requiredGravity) {
							continue;
						}
					}

					FloatInterval xSpeed, ySpeed;
					c.GetSpeedRange(xSpeed, ySpeed, c_el_id.goingUp);

					CornerWaypoint nextWp;
					nextWp.corner_id = c_el_id;
					nextWp.playerState.inverseGravity = prevState.inverseGravity;
					nextWp.playerState.pos = elConnRegion;
					nextWp.playerState.vx = xSpeed;
					nextWp.playerState.vy = ySpeed;

					// Must connect to corner from region before it
					// TODO: is this correct? i think not quite, since we could connect from the intermediate region as well
					Region tmp = Region(c.GetRegionBefore(c_el_id.goingUp)).join(c.GetIntermediateRegion());
					prevState.pos.intersect(tmp);

					// Restrict regions by how they connect
					ReduceByFliplessConnectivity(prevState.pos, nextWp.playerState.pos, prevState.vx, prevState.vy, prevState.inverseGravity, false);
					// ReduceRangesByConnectivity(prevState, nextWp.playerState, frame, WallID::invalid());

					// Add this path to list
					WaypointPath newPath(history);
					newPath.getLastPlayerState() = prevState;
					newPath.target = nextWp;
					new_paths.push_back(newPath);
				} else {
					// Not yet implemented waypoint types
					Exceptions::unimplemented();
				}
			}
		}

		// TODO: reduce by connectivity across entire path

		// Iterate over the new paths, making recursive calls if necessary
		for (std::vector<WaypointPath>::const_iterator it = new_paths.begin(); it != new_paths.end(); it++) {
			const WaypointPath& path = *it;
			if (path.getLastPlayerStateConst().is_bottom()) {
				continue;
			}
			
			if (path.is_partial()) {
				std::vector<WaypointPath> rec_results = NewRecursiveSurfaceConnections(frame, path, elements);
				results.insert(results.end(), rec_results.begin(), rec_results.end());
			} else {
				results.emplace_back(path);
			}
		}

		return results;
	}

	std::vector<ElementRegion> GetUncoveredRanges(const LocalFrame& frame, const WaypointPath& history, const std::set<GenericID>& elements) {
		std::vector<ElementRegion> coveredParts;
		bool hasWalkingConn = false;

		Region fullMustRegion(history.source.playerState.pos);
		Region fullMayRegion(history.source.playerState.pos);

		PlayerStateRange mustState = history.source.playerState;
		PlayerStateRange mayState = history.source.playerState;

		GenericWaypoint& prevPastWp = GenericWaypoint(history.source);
		for (std::vector<GenericWaypoint>::const_iterator it2 = history.waypoints.cbegin(); it2 != history.waypoints.cend(); it2++) {
			const GenericWaypoint& pastWp = *it2;

			bool toSurface = pastWp.id.isWall();
			bool isWalkingConn = toSurface && pastWp.id == prevPastWp.id;

			const PlayerStateRange& fromState = prevPastWp.playerState;
			const PlayerStateRange& toState = pastWp.playerState;

			const Region connRegion = GetConnectingRegionInLocalFrame(pastWp.id, frame);

			if (isWalkingConn) {
				// Must state remains the same
				PlayerStateRange newMustState(mustState);
				newMustState.inverseGravity = toState.inverseGravity;
				// newMustState.pos.x += IntInterval(-MAX_X_SPEED, MAX_X_SPEED);

				// May state encompasses entire platform
				PlayerStateRange newMayState(toState);
				newMayState.pos.join(connRegion);
				newMayState.vx.join(FULL_X_SPEED_RANGE);
				newMayState.vy.join(Y_SPEED_RANGE_FOR_GRAVITY(mayState.inverseGravity));

				mustState = newMustState;
				mayState = newMayState;
			} else {
				PlayerStateRange newMustState(toState);
				newMustState.pos.join(connRegion);
				PlayerStateRange newMayState(toState);
				newMayState.pos.join(connRegion);

				ReduceByFliplessConnectivity(mustState.pos, newMustState.pos, mustState.vx, mustState.vy, mustState.inverseGravity, toSurface);
				ReduceByFliplessConnectivity(mayState.pos, newMayState.pos, mayState.vx, mayState.vy, mayState.inverseGravity, toSurface);

				mustState = newMustState;
				mayState = newMayState;
			}

			bool is_new = true;
			for (std::vector<ElementRegion>::iterator it2 = coveredParts.begin(); it2 != coveredParts.end(); it2++) {
				if ((*it2).element_id != pastWp.id) {
					continue;
				}
				if (mustState.pos.x.intersects((*it2).region.x)) {
					is_new = false;
					(*it2).region.join(mustState.pos);
					break;
				}
			}
			if (is_new) {
				coveredParts.emplace_back(pastWp.id, mustState.pos);
			}
			
			prevPastWp = pastWp;
			fullMustRegion.join(mustState.pos);
			fullMayRegion.join(mayState.pos);
		}

		// Sort the covered parts (by element id and ascending x order)
		std::sort(coveredParts.begin(), coveredParts.end());

		// Derive the uncovered parts from the covered ones
		std::vector<ElementRegion> uncoveredParts;

		for (std::set<GenericID>::const_iterator it = elements.cbegin(); it != elements.cend(); it++) {
			const GenericID& el_id = *it;
			Exceptions::assert(el_id.is_valid());
			
			if (el_id.isCorner()) {
				// Don't connect to self
				if (el_id == GenericID(history.source.corner_id)) {
					continue;
				}
				// To connect to a corner, you *must* have been behind it at some point
				const CornerID& c = el_id.unwrapCorner();
				const Corner& corner = GetCornerInLocalFrame(c, frame);
				if (!corner.GetRegionBefore(c.goingUp).intersects(fullMayRegion)) {
					continue;
				}
				// You also may not have already been past it
				if (corner.GetRegionAfter(c.goingUp).intersects(fullMayRegion)) {
					continue;
				}
			}

			const Region& connectingRegion = GetConnectingRegionInLocalFrame(el_id, frame);

			int firstUncoveredX = connectingRegion.x.getLowerBound();
			for (std::vector<ElementRegion>::const_iterator it2 = coveredParts.cbegin(); it2 != coveredParts.cend(); it2++) {
				const ElementRegion& el_reg = *it2;
				if (el_reg.element_id != el_id) {
					// Wrong element
					continue;
				}

				const IntInterval& coveredXRange = el_reg.region.x;
				int lastUncoveredX = coveredXRange.getIntervalBelow().getUpperBound();
				int newFirstUncoveredX = coveredXRange.getIntervalAbove().getLowerBound();

				IntInterval uncoveredXRange(firstUncoveredX, lastUncoveredX);
				firstUncoveredX = newFirstUncoveredX;

				if (!uncoveredXRange.is_bottom()) {
					uncoveredParts.emplace_back(el_id, Region(uncoveredXRange, connectingRegion.y));
				}
			}

			int finalUncoveredX = connectingRegion.x.getUpperBound();
			IntInterval finalUncoveredXRange(firstUncoveredX, finalUncoveredX);
			if (!finalUncoveredXRange.is_bottom()) {
				uncoveredParts.emplace_back(el_id, Region(finalUncoveredXRange, connectingRegion.y));
			}
		}

		return uncoveredParts;
	}

	// This simulates the game frame by frame, so isn't terribly efficient
	bool ReduceRangesByConnectivity(PlayerStateRange& from, PlayerStateRange& to, const LocalFrame& frame, const WallID& wall_id) {
		if (from.inverseGravity != to.inverseGravity) {
			from.pos.make_bottom();
			to.pos.make_bottom();
			return false;
		}
		
		bool inverseGravity = from.inverseGravity;
		Region blockedRegion = wall_id.is_valid() ? GetWallInLocalFrame(wall_id, frame).GetBlockedRegion() : Region::bottom();

		// Maybe a bit too strict but should be fine
		if (from.pos.intersects(blockedRegion) || to.pos.intersects(blockedRegion)) {
			from.pos.make_bottom();
			to.pos.make_bottom();
			return false;
		}

		FloatInterval a_x = FloatInterval(-MAX_X_SPEED, MAX_X_SPEED);
		float a_y = inverseGravity ? -3.0f : 3.0f;
		
		if (a_y >= 0 && from.vy >= 0 && from.pos.getMin().y > to.pos.getMax().y) {
			from.pos.make_bottom();
			to.pos.make_bottom();
			return false;
		} else if (a_y <= 0 && from.vy <= 0 && from.pos.getMax().y > to.pos.getMin().y) {
			from.pos.make_bottom();
			to.pos.make_bottom();
			return false;
		}
		
		PlayerStateRange currentState(from);
		
		Region totalOverlap = Region::bottom();
		FloatInterval vxOverlap = FloatInterval::bottom();
		FloatInterval vyOverlap = FloatInterval::bottom();

		while (true) {
			Region currentOverlap = Region(currentState.pos).intersect(to.pos);
			if (!currentOverlap.is_bottom()) {
				// Found some overlap, add it to the total
				totalOverlap.join(currentOverlap);
				vxOverlap.join(FloatInterval(currentState.vx).intersect(to.vx));
				vyOverlap.join(FloatInterval(currentState.vy).intersect(to.vy));
			}
			// Check if we are done (have we fallen past the desired region or fully covered it?)
			if (inverseGravity) {
				if (currentState.pos.getMax().y < to.pos.getMin().y) {
					break;
				}
			} else {
				if (currentState.pos.getMin().y > to.pos.getMax().y) {
					break;
				}
			}

			// Advance state
			FrameAdvancePlayerStateRange(currentState, blockedRegion);
		}

		to.pos = totalOverlap;
		to.vx = vxOverlap;
		to.vy = vyOverlap;

		// TODO: reduce from region as well
	}

	void FrameAdvancePlayerStateRange(PlayerStateRange& state, const Region& blockedRegion) {
		Exceptions::require(!state.pos.intersects(blockedRegion));

		float a_x_left = -3.0f;
		float a_x_right = 3.0f;
		float a_y = state.inverseGravity ? -3.0f : 3.0f;

		std::vector<FloatInterval> v_x_init;
		std::vector<FloatInterval> v_y_init;
		{
			v_x_init.push_back(state.vx + a_x_left);
			v_x_init.push_back(state.vx + a_x_right);
			v_y_init.push_back(state.vy + a_y);
		}

		std::vector<FloatInterval> v_x_fric;
		std::vector<FloatInterval> v_y_fric;

		// Apply friction (x)
		for (std::vector<FloatInterval>::const_iterator it = v_x_init.cbegin(); it != v_x_init.cend(); it++) {
			const FloatInterval& v_x = *it;
			if (v_x.is_bottom()) {
				continue;
			}
			if (!v_x.is_positive()) {
				v_x_fric.push_back(v_x.negativePart() + X_RATE);
			}
			if (!v_x.is_negative()) {
				v_x_fric.push_back(v_x.positivePart() - X_RATE);
			}
			if (v_x.intersects(0.0f)) {
				v_x_fric.push_back(0.0f);
			}
		}
		// Apply friction (y)
		for (std::vector<FloatInterval>::const_iterator it = v_y_init.cbegin(); it != v_y_init.cend(); it++) {
			const FloatInterval& v_y = *it;
			if (v_y.is_bottom()) {
				continue;
			}
			if (!v_y.is_positive()) {
				v_y_fric.push_back(v_y.negativePart() + Y_RATE);
			}
			if (!v_y.is_negative()) {
				v_y_fric.push_back(v_y.positivePart() - Y_RATE);
			}
			if (v_y.intersects(0.0f)) {
				v_y_fric.push_back(0.0f);
			}
		}

		// Clamp to range (x)
		for (std::vector<FloatInterval>::iterator it = v_x_fric.begin(); it != v_x_fric.end(); it++) {
			FloatInterval& v_x = *it;
			v_x.clamp(FloatInterval(-MAX_X_SPEED, MAX_X_SPEED));
		}
		// Clamp to range (y)
		for (std::vector<FloatInterval>::iterator it = v_y_fric.begin(); it != v_y_fric.end(); it++) {
			FloatInterval& v_y = *it;
			v_y.clamp(FloatInterval(-MAX_Y_SPEED, MAX_Y_SPEED));
		}

		// Come to a stop with low speed
		FloatInterval lowX = FloatInterval::fromUpperBound(-X_RATE);
		FloatInterval highX = FloatInterval::fromLowerBound(X_RATE);
		FloatInterval lowY = FloatInterval::fromUpperBound(-Y_RATE);
		FloatInterval highY = FloatInterval::fromLowerBound(Y_RATE);

		std::vector<FloatInterval> v_x_final;
		std::vector<FloatInterval> v_y_final;
		for (std::vector<FloatInterval>::const_iterator it = v_x_fric.cbegin(); it != v_x_fric.cend(); it++) {
			const FloatInterval& v_x = *it;
			if (v_x.is_bottom()) {
				continue;
			}
			FloatInterval v_x_l = FloatInterval(v_x).intersect(lowX);
			if (!v_x_l.is_bottom()) {
				v_x_final.push_back(v_x_l);
			}
			if (v_x.min < highX.min && v_x.max > lowX.max) {
				v_x_final.push_back(FloatInterval(0.0f));
			}
			FloatInterval v_x_h = FloatInterval(v_x).intersect(highX);
			if (!v_x_h.is_bottom()) {
				v_x_final.push_back(v_x_h);
			}
		}
		for (std::vector<FloatInterval>::const_iterator it = v_y_fric.cbegin(); it != v_y_fric.cend(); it++) {
			const FloatInterval& v_y = *it;
			if (v_y.is_bottom()) {
				continue;
			}
			FloatInterval v_y_l = FloatInterval(v_y).intersect(lowY);
			if (!v_y_l.is_bottom()) {
				v_y_final.push_back(v_y_l);
			}
			if (v_y.min < highY.min && v_y.max > lowY.max) {
				v_y_final.push_back(FloatInterval(0.0f));
			}
			FloatInterval v_y_h = FloatInterval(v_y).intersect(highY);
			if (!v_y_h.is_bottom()) {
				v_y_final.push_back(v_y_h);
			}
		}
		
		// Round to int interval
		IntInterval d_x = IntInterval::bottom();
		IntInterval d_y = IntInterval::bottom();
		FloatInterval acc_v_x = FloatInterval::bottom();
		FloatInterval acc_v_y = FloatInterval::bottom();
		for (std::vector<FloatInterval>::const_iterator it = v_x_final.cbegin(); it != v_x_final.cend(); it++) {
			const FloatInterval& v_x = *it;
			if (v_x.is_bottom()) {
				continue;
			}

			acc_v_x.join(v_x);
			d_x.join(v_x.toIntInterval());
		}
		for (std::vector<FloatInterval>::const_iterator it = v_y_final.cbegin(); it != v_y_final.cend(); it++) {
			const FloatInterval& v_y = *it;
			if (v_y.is_bottom()) {
				continue;
			}

			acc_v_y.join(v_y);
			d_y.join(v_y.toIntInterval());
		}

		// Wall collisions
		const Region leftSide = Region::fromXUpperBound(blockedRegion.getMin().x - 1);
		const Region middleSide = Region::fromXInterval(IntInterval(blockedRegion.getMin().x, blockedRegion.getMax().x));
		const Region rightSide = Region::fromXLowerBound(blockedRegion.getMax().x + 1);

		const Region above = Region::fromYUpperBound(blockedRegion.getMin().y - 1);
		const Region beside = Region::fromYInterval(IntInterval(blockedRegion.getMin().y, blockedRegion.getMax().y));
		const Region below = Region::fromYLowerBound(blockedRegion.getMax().y + 1);

		Region posAbove = Region(state.pos).intersect(above);
		Region posBelow = Region(state.pos).intersect(below);
		Region posBesideLeft = Region(state.pos).intersect(beside).intersect(leftSide);
		Region posBesideRight = Region(state.pos).intersect(beside).intersect(rightSide);

		IntInterval v_x_lim_positive = -posBesideLeft.x + blockedRegion.getMin().x - 1;
		IntInterval v_x_lim_negative = -posBesideRight.x + blockedRegion.getMax().x + 1;

		posAbove.x += d_x;
		posBelow.x += d_x;
		posBesideLeft.x += d_x;
		posBesideRight.x += d_x;

		// Get blocked by x wall
		posBesideLeft.addXUpperBound(blockedRegion.getMin().x - 1);
		posBesideRight.addXLowerBound(blockedRegion.getMax().x + 1);

		Region posAboveLeft = Region(posAbove).intersect(leftSide);
		Region posAboveMiddle = Region(posAbove).intersect(middleSide);
		Region posAboveRight = Region(posAbove).intersect(rightSide);

		Region posBelowLeft = Region(posBelow).intersect(leftSide);
		Region posBelowMiddle = Region(posBelow).intersect(middleSide);
		Region posBelowRight = Region(posBelow).intersect(rightSide);

		IntInterval v_y_lim_positive = -posAboveMiddle.y + blockedRegion.getMin().y - 1;
		IntInterval v_y_lim_negative = -posBelowMiddle.y + blockedRegion.getMax().y + 1;

		posAboveLeft.y += d_y;
		posAboveMiddle.y += d_y;
		posAboveRight.y += d_y;
		posBesideLeft.y += d_y;
		posBesideRight.y += d_y;
		posBelowLeft.y += d_y;
		posBelowMiddle.y += d_y;
		posBelowRight.y += d_y;

		// Get blocked by y wall
		posAboveMiddle.addYUpperBound(blockedRegion.getMin().y - 1);
		posBelowMiddle.addYLowerBound(blockedRegion.getMax().y + 1);

		// Join with limited speeds if necessary
		FloatInterval lim_v_x_pos = FloatInterval::fromIntInterval(v_x_lim_positive);
		FloatInterval lim_v_x_neg = FloatInterval::fromIntInterval(v_x_lim_negative);
		FloatInterval lim_v_y_pos = FloatInterval::fromIntInterval(v_y_lim_positive);
		FloatInterval lim_v_y_neg = FloatInterval::fromIntInterval(v_y_lim_negative);
		if (lim_v_x_pos.min < acc_v_x.max) {
			acc_v_x.join(lim_v_x_pos.min);
		}
		if (lim_v_x_neg.max > acc_v_x.min) {
			acc_v_x.join(lim_v_x_neg.max);
		}
		if (lim_v_y_pos.min < acc_v_y.max) {
			acc_v_y.join(lim_v_y_pos.min);
		}
		if (lim_v_y_neg.max > acc_v_y.min) {
			acc_v_y.join(lim_v_y_neg.max);
		}

		// Return results
		state.pos = posAboveLeft.join(posAboveMiddle).join(posAboveRight).join(posBesideLeft).join(posBesideRight).join(posBelowLeft).join(posBelowMiddle).join(posBelowRight);
		state.vx = acc_v_x;
		state.vy = acc_v_y;
	}

	void FrameReversePlayerStateRange(PlayerStateRange& state, const Region& blockedRegion) {
		// TODO
		// split into to parts: first (or chronologically last), the part on the surface where you walk around
		// then, simulate the fall onto the surface (backwards of course) to the starting position
	}

	void DoFlip(PlayerStateRange& state) {
		state.inverseGravity = !state.inverseGravity;
		state.vy = state.inverseGravity ? -4.0f : 4.0f;
	}
	void DoGravityLineFlip(PlayerStateRange& state) {
		state.inverseGravity = !state.inverseGravity;

		FloatInterval vy_above, vy_below;
		// Note: this sets gravity but not a_y!
		// TODO: i don't think this is accurately reflected by the logic atm!
		if (state.inverseGravity) {
			// if (entities[i].vy > -1) entities[i].vy = -1;
			state.vy.splitAt(vy_above, vy_below, -1, false);
			vy_above.make_exact(-1);
		} else {
			// if (entities[i].vy < 1) entities[i].vy = 1;
			state.vy.splitAt(vy_above, vy_below, 1, true);
			vy_below.make_exact(1);
		}

		state.vy.make_bottom().join(vy_above).join(vy_below);
	}

	// This simulates the game frame by frame, so isn't terribly efficient
	bool ReduceByFliplessConnectivity(Region& from, Region& to, FloatInterval v_x, FloatInterval v_y, bool inverseGravity, bool toSurface) {
		if (from.is_bottom() || to.is_bottom() || v_x.is_bottom() || v_y.is_bottom()) {
			from.make_bottom();
			to.make_bottom();
			return false;
		}
		if (!from.is_bounded() || !to.is_bounded()) {
			// TODO: this is very imprecise
			VVV_exit(-1);
			return true;
		}
		
		const IntInterval delta_x = to.x - from.x;
		const IntInterval delta_y = to.y - from.y;

		if (delta_y.is_bottom() || delta_x.is_bottom()) {
			from.make_bottom();
			to.make_bottom();
			return false;
		}

		const FloatInterval a_x(-3.0f, 3.0f);
		const float a_y = inverseGravity ? -3.0f : 3.0f;

		IntInterval d_x = 0;
		IntInterval d_y = 0;

		int frames_elapsed = 0;
		while (!d_y.intersects(delta_y) || !d_x.intersects(delta_x)) {
			// Check we didn't fall past the end
			if (d_y > delta_y && a_y >= 0 && v_y >= 0) {
				from.make_bottom();
				to.make_bottom();
				return false;
			} else if (d_y < delta_y && a_y <= 0 && v_y <= 0) {
				from.make_bottom();
				to.make_bottom();
				return false;
			}

			IntInterval prev_dx(d_x);
			IntInterval prev_dy(d_y);
			DoIntervalPhysicsStep(a_x, a_y, v_x, v_y, d_x, d_y);

			if (toSurface) {
				if (a_y > 0) {
					int surfaceY = delta_y.max;
					if (delta_x.contains(d_x) && prev_dy < surfaceY) {
						// Don't allow moving past the surface
						d_y.clamp(IntInterval::fromUpperBound(surfaceY));
					} else if (d_x.intersects(delta_x)) {
						// Account for landing on the surface
						if (IntInterval::join(prev_dy, d_y).intersects(surfaceY)) {
							d_y.join(surfaceY);
						}
					}
				} else {
					int surfaceY = delta_y.min;
					if (delta_x.contains(d_x) && prev_dy > surfaceY) {
						// Don't allow moving past the surface
						d_y.clamp(IntInterval::fromLowerBound(surfaceY));
					} else if (d_x.intersects(delta_x)) {
						// Account for landing on the surface
						if (IntInterval::join(prev_dy, d_y).intersects(surfaceY)) {
							d_y.join(surfaceY);
						}
					}
				}
			}
			
			frames_elapsed++;
		}

		// Now we are overlapping, find reachable parts
		Region reachableToRegion = Region::bottom();
		Region startableFromRegion = Region::bottom();

		while (d_y.intersects(delta_y) && d_x.intersects(delta_x)) {
			Region reachableThisFrame = Region::intersect(to, Region(from.x + d_x, from.y + d_y));
			Region startableThisFrame = Region::intersect(from, Region(to.x - d_x, to.y - d_y));

			reachableToRegion.join(reachableThisFrame);
			startableFromRegion.join(startableThisFrame);

			DoIntervalPhysicsStep(a_x, a_y, v_x, v_y, d_x, d_y);
			frames_elapsed++;
		}

		// Adjust the given regions accordingly
		to.intersect(reachableToRegion);
		from.intersect(startableFromRegion);
	}

	void DoIntervalPhysicsStep(const FloatInterval& a_x, const FloatInterval& a_y, FloatInterval& v_x, FloatInterval& v_y, IntInterval& xp, IntInterval& yp) {
		const float x_rate = X_RATE;
		const float y_rate = Y_RATE;

		v_x += a_x;
		v_y += a_y;

		// Apply friction
		v_x = FloatInterval::join(v_x.positivePart() - x_rate, v_x.negativePart() + x_rate);
		v_y = FloatInterval::join(v_y.positivePart() - y_rate, v_y.negativePart() + y_rate);

		// Clamp to valid range
		v_x.clamp(FloatInterval(-MAX_X_SPEED, MAX_X_SPEED));
		v_y.clamp(FloatInterval(-MAX_Y_SPEED, MAX_Y_SPEED));

		// Conditionally set to zero (if less than friction)
		if (!(v_x.abs() >= x_rate) && !v_x.is_bottom()) {
			v_x.join(0.0f);
		}
		if (!(v_y.abs() >= y_rate) && !v_y.is_bottom()) {
			v_y.join(0.0f);
		}

		IntInterval x_step = v_x.toIntInterval();
		IntInterval y_step = v_y.toIntInterval();

		xp += x_step;
		yp += y_step;
	}

	SymbolicPlayerState SymbolicExecution(const SymbolicPlayerState& fromState) {
		using namespace Numerics;
		using namespace Constraints;
		SymbolicPlayerState state(fromState);
		FloatRange a_x, a_y;
		a_x.make_bottom();
		a_y.make_bottom();

		if (state.leftInput.may_be_true()) {
			a_x.join(-3.0f);
		}
		if (state.leftInput.may_be_false()) {
			if (state.rightInput.may_be_true()) {
				a_x.join(3.0f);
			}
			if (state.rightInput.may_be_false()) {
				a_x.join(0.0f);
			}
		}

		// updateentitylogic
		state.v_x = state.v_x + a_x;
		state.v_y = state.v_y + a_y;
		a_x = FloatRange(0);

		if (fromState.inverseGravity.may_be_true()) {
			a_y.join(-3);
		}
		if (fromState.inverseGravity.may_be_false()) {
			a_y.join(3);
		}

		float xrate = 1.1f;
		float yrate = 0.25f;

		{   // if (entities[t].vx > 0.00f) entities[t].vx -= xrate;
			FloatRange v_x_cmp_tru(state.v_x);
			EnforceGtConstraint(v_x_cmp_tru, FloatRange(0.00f), BoolRange(true));
			FloatRange v_x_cmp_fls(state.v_x);
			EnforceGtConstraint(v_x_cmp_fls, FloatRange(0.00f), BoolRange(false));
			v_x_cmp_tru = v_x_cmp_tru - xrate;
			state.v_x.make_bottom().join(v_x_cmp_tru).join(v_x_cmp_fls);
		}
		{  // if (entities[t].vx < 0.00f) entities[t].vx += xrate;
			FloatRange v_x_cmp_tru(state.v_x);
			EnforceLtConstraint(v_x_cmp_tru, FloatRange(0.00f), BoolRange(true));
			FloatRange v_x_cmp_fls(state.v_x);
			EnforceLtConstraint(v_x_cmp_fls, FloatRange(0.00f), BoolRange(false));
			v_x_cmp_tru = v_x_cmp_tru + xrate;
			state.v_x.make_bottom().join(v_x_cmp_tru).join(v_x_cmp_fls);
		}
		{  // if (entities[t].vy > 0.00f) entities[t].vy -= yrate;
			FloatRange v_y_cmp_tru(state.v_y);
			EnforceGtConstraint(v_y_cmp_tru, FloatRange(0.00f), BoolRange(true));
			FloatRange v_y_cmp_fls(state.v_y);
			EnforceGtConstraint(v_y_cmp_fls, FloatRange(0.00f), BoolRange(false));
			v_y_cmp_tru = v_y_cmp_tru - yrate;
			state.v_y.make_bottom().join(v_y_cmp_tru).join(v_y_cmp_fls);
		}
		{  // if (entities[t].vy < 0.00f) entities[t].vy += yrate;
			FloatRange v_y_cmp_tru(state.v_y);
			EnforceLtConstraint(v_y_cmp_tru, FloatRange(0.00f), BoolRange(true));
			FloatRange v_y_cmp_fls(state.v_y);
			EnforceLtConstraint(v_y_cmp_fls, FloatRange(0.00f), BoolRange(false));
			v_y_cmp_tru = v_y_cmp_tru + yrate;
			state.v_y.make_bottom().join(v_y_cmp_tru).join(v_y_cmp_fls);
		}
		{  // if (entities[t].vy > 10.00f) entities[t].vy = 10.0f;
			FloatRange v_y_cmp_fls(state.v_y);
			EnforceGtConstraint(v_y_cmp_fls, FloatRange(10.00f), BoolRange(false));
			state.v_y.make_bottom().join(FloatRange(10.0f)).join(v_y_cmp_fls);
		}
		{  // if (entities[t].vy < -10.00f) entities[t].vy = -10.0f;
			FloatRange v_y_cmp_fls(state.v_y);
			EnforceLtConstraint(v_y_cmp_fls, FloatRange(-10.00f), BoolRange(false));
			state.v_y.make_bottom().join(FloatRange(-10.0f)).join(v_y_cmp_fls);
		}
		{  // if (entities[t].vx > 6.00f) entities[t].vx = 6.0f;
			FloatRange v_x_cmp_fls(state.v_x);
			EnforceGtConstraint(v_x_cmp_fls, FloatRange(6.00f), BoolRange(false));
			state.v_x.make_bottom().join(FloatRange(6.0f)).join(v_x_cmp_fls);
		}
		{  // if (entities[t].vx < -6.00f) entities[t].vx = -6.0f;
			FloatRange v_x_cmp_fls(state.v_x);
			EnforceLtConstraint(v_x_cmp_fls, FloatRange(-6.00f), BoolRange(false));
			state.v_x.make_bottom().join(FloatRange(-6.0f)).join(v_x_cmp_fls);
		}
		{ // if (SDL_fabsf(entities[t].vx) < xrate) entities[t].vx = 0.0f;
			FloatRange abs_vx = abs(state.v_x);
			FloatRange abs_vx_cmp_fls(state.v_x);
			EnforceLtConstraint(abs_vx_cmp_fls, FloatRange(xrate), BoolRange(false));
			
			FloatRange v_x_cmp_fls(state.v_x);
			EnforceAbsConstraint(v_x_cmp_fls, abs_vx_cmp_fls);

			state.v_x.make_bottom().join(FloatRange(0.0f)).join(v_x_cmp_fls);
		}
		{ // if (SDL_fabsf(entities[t].vy) < yrate) entities[t].vy = 0.0f;
			FloatRange abs_vy = abs(state.v_y);
			FloatRange abs_vy_cmp_fls(state.v_y);
			EnforceLtConstraint(abs_vy_cmp_fls, FloatRange(yrate), BoolRange(false));

			FloatRange v_y_cmp_fls(state.v_y);
			EnforceAbsConstraint(v_y_cmp_fls, abs_vy_cmp_fls);

			state.v_y.make_bottom().join(FloatRange(0.0f)).join(v_y_cmp_fls);
		}

		state.x = state.x + state.v_x.toIntRange();
		state.y = state.y + state.v_y.toIntRange();

		// TODO: entitymapcollision hahaha no
		return state;
	}

	// Given fixed endpoints, find connecting surfaces
	void FindConnectingSurfaces(CornerID from_id, CornerID to_id) {

	}

	bool IsInRange(IntVector range, IntVector v) {
		if (range.x < 0) {
			range.x = -range.x;
			v.x = -v.x;
		}
		if (range.y < 0) {
			range.y = -range.y;
			v.y = -v.y;
		}
		return v.x >= 0 && v.y >= 0 && v.x <= range.x && v.y <= range.y;
	}

	void BuildSurfaceConnectionGraph(NavigationNodeID from, NavigationNodeID to, bool invY) {
		GlobalPosition fromPos = GetNodePos(from);
		GlobalPosition toPos = GetNodePos(to);
		NavigationNode& fromNode = GetNavigationNode(from);
		NavigationNode& toNode = GetNavigationNode(to);

		IntVector distance = GetDistanceBetween(fromPos, toPos, invY);
		bool invX = distance.x < 0;
		// TODO: doens't work for invX == true;

		// Gather all surfaces and lines directly above the edge
		// TODO: this isn't strictly correct, in a case like:
		// ___________________
		//         ____
		// the lower surface in the middle will be missed
		std::set<WallID> surfaces_above;
		std::set<LineID> lines_above;
		int x = 0;
		while (x <= distance.x) {
			// Y should be rounded down to give first integer coordinate above the edge
			int max_y = distance.y * x / distance.x;

			GlobalPosition actualPos(fromPos);
			actualPos.pos += IntVector(x, max_y);
			actualPos = DoAllRoomChanges(actualPos);

			int found_item = 0;
			WallID next_surface(actualPos.room, -1);
			LineID next_line(actualPos.room, -1);
			while (true) {
				RoomData& roomData = GetRoomData(actualPos.room);
				int best_y = -1000;
				for (int l = 0; l < roomData.lines.size(); l++) {
					GravityLine& line = roomData.lines.at(l);
					if (line.min.x > actualPos.pos.x || line.max.x < actualPos.pos.x) {
						// Not above the desired x pos
						continue;
					}
					if (line.max.y > actualPos.pos.y || line.max.y <= best_y) {
						// Not above the edge or not better than previous
						continue;
					}
					int d_y = GetDistanceBetween(fromPos, GlobalPosition(actualPos.room, line.max), invY).y;
					int e_y = distance.y;
					if (d_y * distance.x >= line.min.x * e_y && d_y * distance.x <= line.max.x * e_y) {
						VVV_exit(-1);
						// The edge crosses the line, which is not a very nice case to handle
						// TODO: how best to handle this?
						continue;
					}

					best_y = line.max.y;
					next_line.room = actualPos.room;
					next_line.lineIndex = l;
					found_item = 2;
				}
				for (int w = 0; w < roomData.walls.size(); w++) {
					RoomWall& wall = roomData.walls.at(w);
					if (wall.type != Ceiling) {
						// Not a ceiling
						continue;
					}
					if (wall.min > actualPos.pos.x || wall.max < actualPos.pos.x) {
						// Not above the desired x pos
						continue;
					}
					if (wall.plane > actualPos.pos.y || wall.plane <= best_y) {
						// Not above the edge or not better than previous
						continue;
					}
					best_y = wall.plane;
					next_surface.room = actualPos.room;
					next_surface.wallIndex = w;
					found_item = 1;
				}

				if (found_item > 0) {
					// Found it!
					break;
				} else {
					// Go to next room above
					actualPos.room = actualPos.room.NextRoomUp();
					actualPos.pos.y = roomData.GetMaxYPos() + 1;
				}
			}

			int increment = 1;
			if (found_item == 1) {
				RoomWall& s = GetWall(next_surface);
				if (s.walkable) {
					// Only store if we can flip on it
					surfaces_above.insert(next_surface);
				}
				increment += s.max - actualPos.pos.x;
			} else if (found_item == 2) {
				GravityLine& s = GetGravityLine(next_line);
				lines_above.insert(next_line);
				increment += s.max.x - actualPos.pos.x;
			}

			// Go to next uncovered position
			x += increment;
		}

		// Gather all surfaces and lines directly below the edge
		std::set<WallID> surfaces_below;
		std::set<LineID> lines_below;
		x = 0;
		while (x <= distance.x) {
			// Y should be rounded up to give first integer coordinate below the edge
			int min_y = (distance.y * x + distance.x - 1) / distance.x;

			GlobalPosition actualPos(fromPos);
			actualPos.pos += IntVector(x, min_y);
			actualPos = DoAllRoomChanges(actualPos);

			int found_item = 0;
			WallID next_surface(actualPos.room, -1);
			LineID next_line(actualPos.room, -1);
			while (true) {
				RoomData& roomData = GetRoomData(actualPos.room);
				int best_y = 1000;
				for (int l = 0; l < roomData.lines.size(); l++) {
					GravityLine& line = roomData.lines.at(l);
					if (line.min.x > actualPos.pos.x || line.max.x < actualPos.pos.x) {
						// Not above the desired x pos
						continue;
					}
					if (line.min.y < actualPos.pos.y || line.min.y >= best_y) {
						// Not above the edge or not better than previous
						continue;
					}
					int d_y = GetDistanceBetween(fromPos, GlobalPosition(actualPos.room, line.min), invY).y;
					int e_y = distance.y;
					if (d_y * distance.x >= line.min.x * e_y && d_y * distance.x <= line.max.x * e_y) {
						VVV_exit(-1);
						// The edge crosses the line, which is not a very nice case to handle
						// TODO: how best to handle this?
						continue;
					}

					best_y = line.min.y;
					next_line.room = actualPos.room;
					next_line.lineIndex = l;
					found_item = 2;
				}
				for (int w = 0; w < roomData.walls.size(); w++) {
					RoomWall& wall = roomData.walls.at(w);
					if (wall.type != Floor) {
						// Not a floor
						continue;
					}
					if (wall.min > actualPos.pos.x || wall.max < actualPos.pos.x) {
						// Not above the desired x pos
						continue;
					}
					if (wall.plane < actualPos.pos.y || wall.plane >= best_y) {
						// Not above the edge or not better than previous
						continue;
					}
					best_y = wall.plane;
					next_surface.room = actualPos.room;
					next_surface.wallIndex = w;
					found_item = 1;
				}

				if (found_item > 0) {
					// Found it!
					break;
				} else {
					// Go to next room above
					actualPos.room = actualPos.room.NextRoomDown();
					actualPos.pos.y = roomData.GetMinYPos() - 1;
				}
			}

			int increment = 1;
			if (found_item == 1) {
				RoomWall& s = GetWall(next_surface);
				if (s.walkable) {
					// Only store if we can flip on it
					surfaces_below.insert(next_surface);
				}
				increment += s.max - actualPos.pos.x;
			} else if (found_item == 2) {
				GravityLine& s = GetGravityLine(next_line);
				lines_below.insert(next_line);
				increment += s.max.x - actualPos.pos.x;
			}

			// Go to next uncovered position
			x += increment;
		}

		// Temporary: Visualize the gathered lines and surfaces
		// TODO: remove this
		RoomPosition currentRoom = GetCurrentRoomPosition();
		RoomData& currentRoomData = GetRoomData(currentRoom);
		for (int l = 0; l < currentRoomData.lines.size(); l++) {
			LineID l_id(currentRoom, l);
			if (lines_above.count(l_id) + lines_below.count(l_id) > 0) {
				RenderGravityLine(l_id);
			}
		}
		for (int w = 0; w < currentRoomData.walls.size(); w++) {
			WallID w_id(currentRoom, w);
			if (surfaces_above.count(w_id) + surfaces_below.count(w_id) > 0) {
				RenderWall(w_id);
			}
		}

		// TODO: Process the surfaces usefully
		int totalAbove = lines_above.size() + surfaces_above.size();
		int totalBelow = lines_below.size() + surfaces_below.size();

		// Note: touching a line effectively reduces speed to 1.75f in direction of travel (and inverts gravity)
		// Note: we can basically assume the lowest top surface is above the highest bottom surface (will almost always be the case)

		/*
		struct SurfaceConnection {
			bool invStart;
			bool invEnd;
			int fromIndex;
			int toIndex;
			Interval fromRange;
			Interval toRange;
		};*/

		return;
		LineID fromLine_id = *lines_above.begin();
		LineID toLine_id = *lines_below.begin();

		GravityLine& fromLine = GetGravityLine(fromLine_id);
		GravityLine& toLine = GetGravityLine(toLine_id);

		GlobalPosition ref(fromLine_id.room, fromLine.min);
		LocalFrame local(ref, false, false);

		IntVector fromLineMin = IntVector(0, 0);
		IntVector fromLineMax = ToLocalCoords(local, GlobalPosition(fromLine_id.room, fromLine.max));
		
		GravityLine& toLineRelative = GetGravityLineInLocalFrame(toLine_id, local);
		IntVector toLineMin = toLineRelative.min;
		IntVector toLineMax = toLineRelative.max;

		int y_dist = SDL_min(SDL_abs(toLineMin.y - fromLineMax.y), SDL_abs(toLineMax.y - fromLineMin.y));
		int overlapMin = SDL_max(fromLineMin.x, toLineMin.x);
		int overlapMax = SDL_min(fromLineMax.x, toLineMax.x);

		// Interval fromRange(overlapMin, overlapMax);
		// Interval toRange(overlapMin, overlapMax);
		if (overlapMin <= overlapMax) {
			// Definitely a connection
			IntVector overlapStart(overlapMin, fromLineMax.y);
			IntVector overlapEnd(overlapMax, toLineMin.y);

			GlobalPosition rectStart = ToGlobalCoords(local, overlapStart);
			GlobalPosition rectEnd = ToGlobalCoords(local, overlapEnd);

			RenderRect(rectStart, rectEnd);
		}

		// TODO
	}

	void VisualizeHeuristic(void) {
		struct HNode {
			NavigationNodeID node;
			int heuristic;

			HNode(NavigationNodeID node, int heuristic) : node(node), heuristic(heuristic) {}

			bool operator== (const HNode& other) const {
				return node == other.node;
			}
			bool operator!= (const HNode& other) const {
				return !(*this == other);
			}
			bool operator< (const HNode& other) const {
				return heuristic < other.heuristic;
			}
			bool operator<=(const HNode& other) const {
				return heuristic <= other.heuristic;
			}
			bool operator>=(const HNode& other) const {
				return !(*this < other);
			}
			bool operator> (const HNode& other) const {
				return !(*this <= other);
			}
		};
		std::vector<HNode> h_nodes;
		std::set<NavigationNodeID> visited;
		std::priority_queue<HNode, std::vector<HNode>, std::greater<HNode>> queue;

		queue.emplace(goal_node, 0);

		while (!queue.empty()) {
			HNode node = queue.top();
			queue.pop();

			if (visited.count(node.node) > 0) {
				continue;
			}
			visited.insert(node.node);
			h_nodes.push_back(node);

			for (int e = 0; e < edges.size(); e++) {
				NavigationEdge& edge = edges.at(e);
				if (edge.to == node.node) {
					int h = 2;// node.heuristic + Heuristic::basic_heuristic(SDL_abs(edge.distance.x), SDL_abs(edge.distance.y));

					NavigationNodeID other = edge.from;
					queue.emplace(other, h);
				}
			}
		}
		int x_step = 6;
		int y_step = 6;

		RoomPosition currentRoom = GetCurrentRoomPosition();
		for (int x = 0; x < 320; x += x_step) {
			for (int y = 0; y < 240; y += y_step) {
				IntVector pos(x - VIRIDIAN_CX, y - VIRIDIAN_CY);
				if (GetCurrentRoomPlayerCollisionAt(pos) > 0) {
					continue;
				}
				GlobalPosition gp(currentRoom, pos);
				
				int min_heuristic = INT_MAX;
				int min_node_index = -1;
				for (int n = 0; n < h_nodes.size(); n++) {
					HNode& node = h_nodes.at(n);
					IntVector distance = GetMinDistanceBetween(gp, GetNodePos(node.node));
					int h = node.heuristic + 2; // Heuristic::basic_heuristic(SDL_abs(distance.x), SDL_abs(distance.y));
					if (h >= min_heuristic) {
						continue;
					}
					float t = GlobalRaycast(currentRoom, Ray(pos.x, pos.y, distance.x, distance.y));
					if (t < 1.0f) {
						continue;
					}

					min_heuristic = h;
					min_node_index = n;
				}

				if (min_heuristic < INT_MAX) {
					HNode& min_node = h_nodes.at(min_node_index);
					int color_val = min_heuristic * 20;

					color_val %= (255 * 2);
					int r = SDL_min(255, color_val);
					int g = SDL_max(0, 255 - SDL_max(0, color_val - 255));
					SDL_SetRenderDrawColor(gameScreen.m_renderer, r, g, 0, 150);
					// RenderPixel(pos.x, pos.y);
					RenderRect(gp, GlobalPosition(currentRoom, pos + IntVector(x_step - 1, y_step - 1)));
				}
			}
		}

		return;
	}

	void FindConnectingCorners(CornerID corner_id, bool inverseGravity) {
		RoomData& roomData = GetRoomData(corner_id.room);
		Corner& corner = GetCorner(corner_id);

		int signedVerticalGapAfter, signedHorizontalGapAfter;
		switch (corner.type) {
		default:
			return;
		case TopLeft:
			if (inverseGravity) {
				signedVerticalGapAfter = -corner.verticalGap;
				signedHorizontalGapAfter = corner.horizontalNegativeGap;
			} else {
				signedVerticalGapAfter = corner.verticalNegativeGap;
				signedHorizontalGapAfter = -corner.horizontalGap;
			}
			break;
		case TopRight:
			if (inverseGravity) {
				signedVerticalGapAfter = -corner.verticalGap;
				signedHorizontalGapAfter = -corner.horizontalNegativeGap;
			} else {
				signedVerticalGapAfter = corner.verticalNegativeGap;
				signedHorizontalGapAfter = corner.horizontalGap;
			}
			break;
		case BottomLeft:
			if (inverseGravity) {
				signedVerticalGapAfter = -corner.verticalNegativeGap;
				signedHorizontalGapAfter = -corner.horizontalGap;
			} else {
				signedVerticalGapAfter = corner.verticalGap;
				signedHorizontalGapAfter = corner.horizontalNegativeGap;
			}
			break;
		case BottomRight:
			if (inverseGravity) {
				signedVerticalGapAfter = -corner.verticalNegativeGap;
				signedHorizontalGapAfter = corner.horizontalGap;
			} else {
				signedVerticalGapAfter = corner.verticalGap;
				signedHorizontalGapAfter = -corner.horizontalNegativeGap;
			}
			break;
		}

		IntVector afterRegionSize(signedHorizontalGapAfter, signedVerticalGapAfter);

	}

	GlobalPosition GetNodePos(NavigationNodeID node_id) {
		NavigationNode& node = GetNavigationNode(node_id);
		GlobalPosition result;
		result.room = node_id.room;
		switch (node.type) {
			default:
				VVV_exit(-1);
			case StartNodeType:
				result.pos = node.data.start.pos;
				break;
			case GoalNodeType:
				result.pos = node.data.goal.pos;
				break;
			case CornerNodeType:
				Corner& c = GetCorner(node.data.corner.corner);
				result.pos = c.pos;
		}
		return result;
	}

	// The min number of frames that need to elapse before walking past this distance
	int GetMinXFrames(int d_x) {
		if (d_x < 0) {
			return 0;
		} else {
			return 1 + (d_x / MAX_X_SPEED);
		}
	}
	// The max number of frames that need to elapse before walking past this distance (assuming constantly moving in that direction)
	int GetMaxXFrames(int d_x) {
		if (d_x < 0) {
			return 0;
		} else if (d_x <= 1 + 3 + 5) {
			if (d_x <= 0) {
				return 1;
			} else if (d_x <= 1) {
				return 2;
			} else if (d_x <= 1 + 3) {
				return 3;
			} else {
				return 4;
			}
		} else {
			return 4 + (d_x - (1 + 3 + 5)) / MAX_X_SPEED;
		}
	}

	IntInterval GetYFrames(IntInterval& d_y, bool inverseGravity) {
		if (d_y.is_bottom()) {
			return IntInterval::bottom();
		}
		IntInterval distance(d_y);
		if (inverseGravity) {
			distance.negate();
		}

		if (distance.is_negative()) {
			if (distance.contains(0)) {
				return IntInterval(0);
			} else {
				return IntInterval::bottom();
			}
		}
		
		float y_accel = 2.75f;
		FloatInterval y_speed(0.0f, MAX_Y_SPEED);
		int min_frames = 0;

		while (distance > 0 && !y_speed.is_exact()) {
			min_frames++;

			y_speed += y_accel;
			if (!y_speed.is_bottom()) {
				y_speed.min = SDL_min(MAX_Y_SPEED, y_speed.min);
				y_speed.max = SDL_min(MAX_Y_SPEED, y_speed.max);
			}
			IntInterval d_y = y_speed.toIntInterval();
			distance -= d_y;
		}
		if (distance > 0) {
			IntInterval d_y = y_speed.toIntInterval();
			if (!d_y.is_exact()) {
				VVV_exit(-1);
			}
			int d_y_exact = d_y.min;

			int f = (distance.min + d_y_exact - 1) / d_y_exact;
			min_frames += f;
			distance -= f * d_y_exact;
		}

		if (!distance.has_upper_bound()) {
			return IntInterval::fromLowerBound(min_frames);
		}

		int max_frames = min_frames;
		while (!(distance < 0) && !y_speed.is_exact()) {
			max_frames++;

			y_speed += y_accel;
			if (!y_speed.is_bottom()) {
				y_speed.min = SDL_min(MAX_Y_SPEED, y_speed.min);
				y_speed.max = SDL_min(MAX_Y_SPEED, y_speed.max);
			}
			IntInterval d_y = y_speed.toIntInterval();
			distance -= d_y;
		}
		if (!(distance < 0)) {
			IntInterval d_y = y_speed.toIntInterval();
			if (!d_y.is_exact()) {
				VVV_exit(-1);
			}
			int d_y_exact = d_y.min;

			int f = (distance.max + d_y_exact - 1) / d_y_exact;
			max_frames += f;
			distance -= f * d_y_exact;
		} else {
			max_frames -= 1;
		}

		return IntInterval(min_frames, max_frames);
	}

	IntInterval GetYDist(int y_frames, bool inverseGravity) {
		float y_accel = 2.75f;
		FloatInterval y_speed(0.0f, MAX_Y_SPEED);
		IntInterval d_y(0);

		while (y_frames > 0 && !y_speed.is_exact()) {
			y_speed += y_accel;
			if (!y_speed.is_bottom()) {
				y_speed.min = SDL_min(MAX_Y_SPEED, y_speed.min);
				y_speed.max = SDL_min(MAX_Y_SPEED, y_speed.max);
			}

			d_y += y_speed.toIntInterval();
			y_frames--;
		}
		if (y_frames > 0) {
			d_y += y_frames * MAX_Y_SPEED;
		}
		return inverseGravity ? -d_y : d_y;
	}
	IntInterval GetYDist(IntInterval& y_frames, bool inverseGravity) {
		if (y_frames.is_bottom() || y_frames.min < 0) {
			return IntInterval::bottom();
		}

		IntInterval minFramesResult = GetYDist(y_frames.min, inverseGravity);
		if (!y_frames.has_upper_bound()) {
			minFramesResult.max = INT_MAX;
			return minFramesResult;
		} else {
			IntInterval maxFramesResult = GetYDist(y_frames.max, inverseGravity);
			return IntInterval::join(minFramesResult, maxFramesResult);
		}
	}

	// The min number of frames that need to elapse before falling past this distance
	int GetMinYFrames(int d_y) {
		if (d_y < 0) {
			return 0;
		} else {
			return 1 + d_y / MAX_Y_SPEED;
		}
	}
	// The max number of frames that need to elapse before falling past this distance
	int GetMaxYFrames(int d_y) {
		if (d_y < 0) {
			return 0;
		} else if (d_y <= 2 + 5 + 8) {
			if (d_y <= 0) {
				return 1;
			} else if (d_y <= 2) {
				return 2;
			} else if (d_y <= 2 + 5) {
				return 3;
			} else {
				return 4;
			}
		} else {
			return 4 + (d_y - (2 + 5 + 8)) / MAX_Y_SPEED;
		}
	}

	IntVector ToLocalCoords(LocalFrame& localFrame, GlobalPosition& globalPos) {
		// invX -> distances are expected to be negative, return positive result
		int d_rx = GetHOffsetBetweenRooms(localFrame.origin.room, globalPos.room);
		// invY -> distances are expected to be negative, return positive result
		int d_ry = localFrame.invY ? GetMinNegativeVOffsetBetweenRooms(localFrame.origin.room, globalPos.room) : GetMinPositiveVOffsetBetweenRooms(localFrame.origin.room, globalPos.room);

		int d_x = d_rx * 320 + (globalPos.pos.x - localFrame.origin.pos.x);
		int d_y = d_ry * 240 + (globalPos.pos.y - localFrame.origin.pos.y);

		return IntVector(localFrame.invX ? -d_x : d_x, localFrame.invY ? -d_y : d_y);
	}

	GlobalPosition ToGlobalCoords(LocalFrame& localFrame, IntVector localPos) {
		GlobalPosition result = localFrame.origin;
		result.pos.x += localFrame.invX ? -localPos.x : localPos.x;
		result.pos.y += localFrame.invY ? -localPos.y : localPos.y;
		return DoAllRoomChanges(result);
	}

	GlobalPosition DoAllRoomChanges(GlobalPosition& globalPos) {
		RoomPosition currentRoom = globalPos.room;
		IntVector currentPos = globalPos.pos;

		bool warped = true;
		while (warped) {
			warped = false;
			RoomData roomData = GetRoomData(currentRoom);

			IntVector min = IntVector(roomData.GetMinXPos(), roomData.GetMinYPos());
			IntVector max = IntVector(roomData.GetMaxXPos(), roomData.GetMaxYPos());

			if (roomData.warpx) {
				// Don't change rooms
				if (currentPos.x < min.x) {
					currentPos.x += 320;
					warped = true;
				} else if (currentPos.x > max.x) {
					currentPos.x -= 320;
					warped = true;
				}
			}

			if (roomData.warpy) {
				// Don't change rooms
				if (currentPos.y < min.y) {
					currentPos.y += 232;
					warped = true;
				} else if (currentPos.y > max.y) {
					currentPos.y -= 232;
					warped = true;
				}
			}

			if (!roomData.warpy) {
				// Normal! Just change room
				if (currentPos.y < min.y) {
					currentPos.y += 240;
					currentRoom.ry = (currentRoom.ry + 19) % 20;
					roomData = GetRoomData(currentRoom);
					warped = true;
				} else if (currentPos.y > max.y) {
					currentPos.y -= 240;
					currentRoom.ry = (currentRoom.ry + 1) % 20;
					roomData = GetRoomData(currentRoom);
					warped = true;
				}
			}

			if (!roomData.warpx) {
				// Normal! Just change room
				if (currentPos.x < min.x) {
					currentPos.x += 320;
					currentRoom.rx = (currentRoom.rx + 19) % 20;
					roomData = GetRoomData(currentRoom);
					warped = true;
				} else if (currentPos.x > max.x) {
					currentPos.x -= 320;
					currentRoom.rx = (currentRoom.rx + 1) % 20;
					roomData = GetRoomData(currentRoom);
					warped = true;
				}
			}
		}

		return GlobalPosition(currentRoom, currentPos);
	}

	GlobalPosition PlayerRoomChangeLogic(GlobalPosition& globalPos) {
		RoomPosition currentRoom = globalPos.room;
		IntVector currentPos = globalPos.pos;

		RoomData roomData = GetRoomData(currentRoom);

		IntVector min = IntVector(roomData.GetMinXPos(), roomData.GetMinYPos());
		IntVector max = IntVector(roomData.GetMaxXPos(), roomData.GetMaxYPos());

		if (roomData.warpx) {
			// Don't change rooms
			if (currentPos.x < min.x) {
				currentPos.x += 320;
			} else if (currentPos.x > max.x) {
				currentPos.x -= 320;
			}
		}

		if (roomData.warpy) {
			// Don't change rooms
			if (currentPos.y < min.y) {
				currentPos.y += 232;
			} else if (currentPos.y > max.y) {
				currentPos.y -= 232;
			}
		}

		if (!roomData.warpy) {
			// Normal! Just change room
			if (currentPos.y < min.y) {
				currentPos.y += 240;
				currentRoom.ry = (currentRoom.ry + 19) % 20;
				roomData = GetRoomData(currentRoom);
			} else if (currentPos.y > max.y) {
				currentPos.y -= 240;
				currentRoom.ry = (currentRoom.ry + 1) % 20;
				roomData = GetRoomData(currentRoom);
			}
		}

		if (!roomData.warpx) {
			// Normal! Just change room
			if (currentPos.x < min.x) {
				currentPos.x += 320;
				currentRoom.rx = (currentRoom.rx + 19) % 20;
				roomData = GetRoomData(currentRoom);
			} else if (currentPos.x > max.x) {
				currentPos.x -= 320;
				currentRoom.rx = (currentRoom.rx + 1) % 20;
				roomData = GetRoomData(currentRoom);
			}
		}

		return GlobalPosition(currentRoom, currentPos);
	}

	// -------------
	// Graph pruning
	// -------------

	int PruneDeadEndEdges(void) {
		int totalRemoved = 0;
		// Prune dead end edges
		int e = 0;
		int numRemoved = 0;
		while (e < edges.size() || numRemoved > 0) {
			if (e >= edges.size()) {
				e = 0;
				numRemoved = 0;

				if (e >= edges.size()) {
					break;
				}
			}

			NavigationEdge& edge = edges.at(e);

			NavigationNode& fromNode = GetRoomData(edge.from.room).nodes.at(edge.from.nodeIndex);
			NavigationNode& toNode = GetRoomData(edge.to.room).nodes.at(edge.to.nodeIndex);

			bool connectIn = fromNode.type == StartNodeType;
			bool connectOut = toNode.type == GoalNodeType;
			for (int e2 = 0; (e2 < edges.size()) && (!connectIn || !connectOut); e2++) {
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

	int PruneBackAndCrossedEdges(void) {
		// Basically: Each edge is a node in the graph, use their connectivity to create edges and then run a dominator algorithm
		// Using the dominators, prune edges that can only connect to another edge they are dominated by, or edges that are crossed (geometrically) by an edge they are dominated by
		// Edges only count as crossed if they have the same gravity -> otherwise the assumption of being able to shortcut isn't guaranteed

		// Edges are identified by their indices in the edges vector
		std::vector<std::vector<int>> predecessors;
		std::vector<std::vector<int>> successors;
		int num_edges = edges.size();
		for (int e = 0; e < num_edges; e++) {
			NavigationEdge& edge = edges.at(e);

			successors.emplace_back();
			predecessors.emplace_back();
			
			for (int o = 0; o < num_edges; o++) {
				if (o == e) {
					continue;
				}
				NavigationEdge& other = edges.at(o);

				if (CanConnectEdges(edge, other)) {
					successors.back().push_back(o);
				}
				if (CanConnectEdges(other, edge)) {
					predecessors.back().push_back(o);
				}
			}
		}

		// Now we'll add an extra start and goal "edge" for the dominator algorithm
		int start_edge_index = predecessors.size();
		successors.emplace_back();
		predecessors.emplace_back();
		int goal_edge_index = predecessors.size();
		successors.emplace_back();
		predecessors.emplace_back();
		for (int e = 0; e < num_edges; e++) {
			NavigationEdge& edge = edges.at(e);

			NavigationNode& fromNode = GetNavigationNode(edge.from);
			NavigationNode& toNode = GetNavigationNode(edge.to);

			if (fromNode.type == StartNodeType) {
				successors.at(start_edge_index).push_back(e);
				predecessors.at(e).push_back(start_edge_index);
			}
			if (toNode.type == GoalNodeType) {
				successors.at(e).push_back(goal_edge_index);
				predecessors.at(goal_edge_index).push_back(e);
			}
		}

		// Initialize dominator sets
		std::vector<std::set<int>> dominator_sets;
		std::vector<std::set<int>> reverse_dominator_sets;
		for (int i = 0; i < predecessors.size(); i++) {
			dominator_sets.emplace_back();
			reverse_dominator_sets.emplace_back();
			if (i != start_edge_index) {
				for (int j = 0; j < predecessors.size(); j++) {
					dominator_sets.back().insert(j);
				}
			} else {
				// Start node is only dominated by itself
				dominator_sets.back().insert(i);
			}
			if (i != goal_edge_index) {
				for (int j = 0; j < predecessors.size(); j++) {
					reverse_dominator_sets.back().insert(j);
				}
			}
			else {
				// Goal node is only dominated by itself
				reverse_dominator_sets.back().insert(i);
			}
		}
		
		// Run the actual algorithm
		bool anythingChanged = true;
		while (anythingChanged) {
			anythingChanged = false;
			for (int i = 0; i < predecessors.size(); i++) {
				std::set<int> new_dominator_set;
				std::set<int> new_reverse_dominator_set;
				
				// Intersection of all predecessor dominator sets
				int num_predecessors = predecessors.at(i).size();
				int p = 0;
				if (p < num_predecessors) {
					int predecessor = predecessors.at(i).at(p);
					new_dominator_set.insert(dominator_sets.at(predecessor).begin(), dominator_sets.at(predecessor).end());
					p++;
					while (p < num_predecessors) {
						predecessor = predecessors.at(i).at(p);
						std::set<int> results;
						std::set_intersection(new_dominator_set.begin(), new_dominator_set.end(), dominator_sets.at(predecessor).begin(), dominator_sets.at(predecessor).end(), std::inserter(results, results.begin()));
						new_dominator_set.clear();
						new_dominator_set.insert(results.begin(), results.end());
						p++;
					}
				}
				// Intersection of all successor reverse dominator sets
				int num_successors = successors.at(i).size();
				int s = 0;
				if (s < num_successors) {
					int successor = successors.at(i).at(s);
					new_reverse_dominator_set.insert(reverse_dominator_sets.at(successor).begin(), reverse_dominator_sets.at(successor).end());
					s++;
					while (s < num_successors) {
						successor = successors.at(i).at(s);
						std::set<int> results;
						std::set_intersection(new_reverse_dominator_set.begin(), new_reverse_dominator_set.end(), reverse_dominator_sets.at(successor).begin(), reverse_dominator_sets.at(successor).end(), std::inserter(results, results.begin()));
						new_reverse_dominator_set.clear();
						new_reverse_dominator_set.insert(results.begin(), results.end());
						s++;
					}
				}

				// Every node dominates itself
				new_dominator_set.insert(i);
				new_reverse_dominator_set.insert(i);

				// Check if anything changed
				bool shouldCopy = (new_dominator_set.size() != dominator_sets.at(i).size()) || (new_reverse_dominator_set.size() != reverse_dominator_sets.at(i).size());
				if (!shouldCopy) {
					// Element-wise check
					for (std::set<int>::iterator it = new_dominator_set.begin(); it != new_dominator_set.end(); ++it) {
						int predecessor = *it;
						if (dominator_sets.at(i).count(predecessor) == 0) {
							// This element is new
							shouldCopy = true;
							break;
						}
					}
					for (std::set<int>::iterator it = new_reverse_dominator_set.begin(); it != new_reverse_dominator_set.end(); ++it) {
						int successor = *it;
						if (reverse_dominator_sets.at(i).count(successor) == 0) {
							// This element is new
							shouldCopy = true;
							break;
						}
					}
				}

				if (shouldCopy) {
					// Copy new dominator set to set list
					dominator_sets.at(i).clear();
					dominator_sets.at(i).insert(new_dominator_set.begin(), new_dominator_set.end());
					reverse_dominator_sets.at(i).clear();
					reverse_dominator_sets.at(i).insert(new_reverse_dominator_set.begin(), new_reverse_dominator_set.end());
					anythingChanged = true;
				}
			}
		}

		std::set<int> edges_to_remove;
		for (int i = 0; i < predecessors.size(); i++) {
			if (i == start_edge_index || i == goal_edge_index) {
				// Ignore our "fake" edges
				continue;
			}
			NavigationEdge& edge = edges.at(i);

			for (std::set<int>::iterator it = dominator_sets.at(i).begin(); it != dominator_sets.at(i).end(); ++it) {
				int d = *it;
				if (d == start_edge_index || d == goal_edge_index) {
					// Ignore our "fake" edges
					continue;
				}
				if (i == d) {
					// Ignore edge dominating itself
					continue;
				}

				NavigationEdge& dominator = edges.at(d);
				// Do the edges cross? in that case we can delete the dominated edge
				if (DoEdgesCross(dominator, edge)) {
					edges_to_remove.insert(i);
					break;
				}
			}

			for (std::set<int>::iterator it = reverse_dominator_sets.at(i).begin(); it != reverse_dominator_sets.at(i).end(); ++it) {
				int d = *it;
				if (d == start_edge_index || d == goal_edge_index) {
					// Ignore our "fake" edges
					continue;
				}
				if (i == d) {
					// Ignore edge dominating itself
					continue;
				}

				NavigationEdge& dominator = edges.at(d);
				// Do the edges cross? in that case we can delete the dominated edge
				if (DoEdgesCross(dominator, edge)) {
					edges_to_remove.insert(i);
					break;
				}
			}

			// Is the edge dominated by all its successors? If so, delete it (it's a backwards edge)
			bool nonDomSuccessors = false;
			for (int s = 0; s < successors.at(i).size(); s++) {
				int successor = successors.at(i).at(s);

				if (dominator_sets.at(i).count(successor) == 0) {
					nonDomSuccessors = true;
					break;
				}
			}

			if (!nonDomSuccessors) {
				edges_to_remove.insert(i);
			}
		}

		int numRemoved = 0;
		for (int e = edges.size() - 1; e >= 0; e--) {
			if (edges_to_remove.count(e) > 0) {
				RemoveEdge(e);
				numRemoved++;
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
	GlobalPosition GetPlayerPosition() {
		IntVector localPos = IntVector(obj.entities[0].xp, obj.entities[0].yp);
		RoomPosition currentRoom = GetCurrentRoomPosition();
		return GlobalPosition(currentRoom, localPos);
	}
	IntVector GetDistanceOffsetBetweenRooms(RoomPosition from, RoomPosition to, bool invY) {
		int d_rx = GetHOffsetBetweenRooms(from, to);
		int d_ry = GetVOffsetBetweenRooms(from, to, invY);
		return IntVector(320 * d_rx, 240 * d_ry);
	}
	IntVector GetMinDistanceOffsetBetweenRooms(RoomPosition from, RoomPosition to) {
		int d_rx = GetHOffsetBetweenRooms(from, to);
		int d_ry = GetMinVOffsetBetweenRooms(from, to);
		return IntVector(320 * d_rx, 240 * d_ry);
	}
	IntVector GetMinOffsetBetweenRooms(RoomPosition from, RoomPosition to) {
		int d_rx = GetHOffsetBetweenRooms(from, to);
		int d_ry = GetMinVOffsetBetweenRooms(from, to);
		return IntVector(d_rx, d_ry);
	}
	IntVector GetDistanceBetween(const GlobalPosition& from, const GlobalPosition& to, bool invY) {
		return GetDistanceOffsetBetweenRooms(from.room, to.room, invY) + to.pos - from.pos;
	}
	IntVector GetMinDistanceBetween(const GlobalPosition& from, const GlobalPosition& to) {
		return GetMinDistanceOffsetBetweenRooms(from.room, to.room) + to.pos - from.pos;
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
	int GetVOffsetBetweenRooms(RoomPosition from, RoomPosition to, bool invY) {
		return invY ? GetMinNegativeVOffsetBetweenRooms(from, to) : GetMinPositiveVOffsetBetweenRooms(from, to);
	}
	int GetMinVOffsetBetweenRooms(RoomPosition from, RoomPosition to) {
		int neg_offset = GetMinNegativeVOffsetBetweenRooms(from, to);
		int pos_offset = GetMinPositiveVOffsetBetweenRooms(from, to);

		if (SDL_abs(neg_offset) < pos_offset) {
			return neg_offset;
		} else {
			return pos_offset;
		}
	}
	int GetMinPositiveVOffsetBetweenRooms(RoomPosition from, RoomPosition to) {
		int d_ry = to.ry - from.ry;
		while (d_ry < 0) {
			d_ry += 20;
		}
		while (d_ry > 19) {
			d_ry -= 20;
		}

		return d_ry;
	}
	int GetMinNegativeVOffsetBetweenRooms(RoomPosition from, RoomPosition to) {
		int d_ry = to.ry - from.ry;
		while (d_ry > 0) {
			d_ry -= 20;
		}
		while (d_ry < -19) {
			d_ry += 20;
		}

		return d_ry;
	}

	uint8_t GetPlayerCollisionAt(GlobalPosition pos) {
		// Load the right room
		LoadRoom(pos.room);

		// Player hitbox
		const SDL_Rect temprect = { pos.pos.x + VIRIDIAN_CX, pos.pos.y + VIRIDIAN_CY, VIRIDIAN_W, VIRIDIAN_H };

		uint8_t collision = 0;
		// Check walls
		if (collisionSetting == CollisionSetting::Walls || collisionSetting == CollisionSetting::WallsAndSpikes) {
			if (obj.checkwall(false, temprect)) {
				// 1: wall
				collision = 1;
			}
		}
		// Check spikes
		if (collisionSetting == CollisionSetting::WallsAndSpikes) {
			for (size_t j = 0; j < obj.blocks.size(); j++) {
				if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect)) {
					// 2: damage
					collision = 2;
				}
			}
		}

		// Return result
		return collision;
	}
	uint8_t GetCurrentRoomPlayerCollisionAt(IntVector pos) {
		// Player hitbox
		const SDL_Rect temprect = { pos.x + VIRIDIAN_CX, pos.y + VIRIDIAN_CY, VIRIDIAN_W, VIRIDIAN_H };

		uint8_t collision = 0;
		// Check walls
		if (collisionSetting == CollisionSetting::Walls || collisionSetting == CollisionSetting::WallsAndSpikes) {
			if (obj.checkwall(false, temprect)) {
				// 1: wall
				collision = 1;
			}
		}
		// Check spikes
		if (collisionSetting == CollisionSetting::WallsAndSpikes) {
			for (size_t j = 0; j < obj.blocks.size(); j++) {
				if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect)) {
					// 2: damage
					collision = 2;
				}
			}
		}

		// Return result
		return collision;
	}

	uint8_t* GetCurrentRoomPlayerCollisionBitmap(IntVector min, IntVector max) {
		int x_extent = max.x - min.x + 1;
		int y_extent = max.y - min.y + 1;

		uint8_t* bitmap = (uint8_t*) SDL_malloc(x_extent * y_extent * sizeof(uint8_t));

		for (int x = min.x; x <= max.x; x++) {
			for (int y = min.y; y <= max.y; y++) {
				// Player hitbox
				const SDL_Rect temprect = { x + VIRIDIAN_CX, y + VIRIDIAN_CY, VIRIDIAN_W, VIRIDIAN_H };

				uint8_t collision = 0;
				// Check walls
				if (collisionSetting == CollisionSetting::Walls || collisionSetting == CollisionSetting::WallsAndSpikes) {
					if (obj.checkwall(false, temprect)) {
						// 1: wall
						collision = 1;
					}
				}
				// Check spikes
				if (collisionSetting == CollisionSetting::WallsAndSpikes) {
					for (size_t j = 0; j < obj.blocks.size(); j++) {
						if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect)) {
							// 2: damage
							collision = 2;
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

	bool RoomPosition::IsTower() {
		if (outside) {
			if (rx == 8) {
				// Panic Room
				return ry == 4 || ry == 5;
			} else if (rx == 10) {
				// Final Challenge
				return ry == 5 || ry == 6;
			}
		} else {
			// Tower
			return rx == TOWER_RX;
		}
		return false;
	}
}
