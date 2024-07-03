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
	RoomCoords start_room = RoomCoords(2, 16);
	int start_x = 10;
	int start_y = 100;

	RoomCoords goal_room = RoomCoords(3, 4);
	int goal_x = 100;
	int goal_y = 160;

	int num_precomputed_rooms = 0;
	RoomInfo precomputed_rooms[20][20] = { { RoomInfo() } };

	CollisionKind collision_kind = CollisionKind::Walls;
	std::vector<NavCorner> nav_corners;
	std::vector<Wall> nav_walls;

	RoomInfoState::RoomInfoState GetRoomState(RoomCoords room_coords) {
		return precomputed_rooms[room_coords.rx % 20][room_coords.ry % 20].state;
	}

	void PrecomputeCurrentRoomDims(void) {
		RoomCoords current_room = GetCurrentRoomCoords();
		if (GetRoomState(current_room) != RoomInfoState::None) {
			VVV_exit(1);
			return;
		}

		int rx = current_room.rx;
		int ry = current_room.ry;

		// Min and max positions the player can stand at in this room
		int min_x_pos, min_y_pos, max_x_pos, max_y_pos;
		if (map.towermode) {
			VVV_exit(-1);
		} else {
			if (map.warpx) {
				min_x_pos = -9; max_x_pos = 310;
			}
			else {
				min_x_pos = -14; max_x_pos = 307;
			}
			if (map.warpy) {
				min_y_pos = -11; max_y_pos = 226;
			}
			else {
				min_y_pos = -2; max_y_pos = 237;
			}
		}

		precomputed_rooms[rx][ry].min_x_pos = min_x_pos;
		precomputed_rooms[rx][ry].max_x_pos = max_x_pos;
		precomputed_rooms[rx][ry].min_y_pos = min_y_pos;
		precomputed_rooms[rx][ry].max_y_pos = max_y_pos;
		precomputed_rooms[rx][ry].warpx = map.warpx;
		precomputed_rooms[rx][ry].warpy = map.warpy;
		precomputed_rooms[rx][ry].towermode = map.towermode;

		precomputed_rooms[rx][ry].state = RoomInfoState::Dimensions;
	}

	void PrecomputeCurrentRoomWalls(void) {
		RoomCoords current_room = GetCurrentRoomCoords();
		if (GetRoomState(current_room) != RoomInfoState::Dimensions) {
			VVV_exit(1);
			return;
		}
		int rx = current_room.rx;
		int ry = current_room.ry;

		int min_x_pos = precomputed_rooms[rx][ry].min_x_pos;
		int max_x_pos = precomputed_rooms[rx][ry].max_x_pos;
		int min_y_pos = precomputed_rooms[rx][ry].min_y_pos;
		int max_y_pos = precomputed_rooms[rx][ry].max_y_pos;

		// Create all corners
		for (int x = min_x_pos; x <= max_x_pos; x++) {
			for (int y = min_y_pos; y <= max_y_pos; y++) {
				int x_left = x - 1;
				int y_up = y - 1;

				bool is_air = !CheckPlayerCollisionCurrentRoom(x, y);
				bool up_is_air = !CheckPlayerCollisionCurrentRoom(x, y_up);
				bool left_is_air = !CheckPlayerCollisionCurrentRoom(x_left, y);
				bool upleft_is_air = !CheckPlayerCollisionCurrentRoom(x_left, y_up);

				int count = is_air + up_is_air + left_is_air + upleft_is_air;

				if (count == 0 || count == 4) {
					// Not a corner, continue
					continue;
				}
				if (count == 2 && (is_air == left_is_air || is_air == up_is_air)) {
					// Not a corner, continue
					continue;
				}

				NavCornerType type = InvalidCorner;
				if (count == 3) {
					// Convex
					if (!is_air) {
						type = TopLeft;
					}
					else if (!up_is_air) {
						type = BottomLeft;
					}
					else if (!upleft_is_air) {
						type = BottomRight;
					}
					else {
						// assume(!left_is_air)
						type = TopRight;
					}
				} else if (count == 1) {
					// Concave
					if (is_air) {
						type = ConcaveBR;
					}
					else if (up_is_air) {
						type = ConcaveTR;
					}
					else if (upleft_is_air) {
						type = ConcaveTL;
					}
					else {
						// assume(left_is_air)
						type = ConcaveBL;
					}
				} else {
					// assume(count == 2)
					if (is_air && upleft_is_air) {
						type = QuadTLBR;
					}
					else if (up_is_air && left_is_air) {
						type = QuadBLTR;
					}
					else {
						// unreachable
						VVV_exit(1);
					}
				}

				// Add corner to list
				nav_corners.emplace_back(type, rx, ry, x, y);
			}
		}

		// Create vertical walls
		for (int x = min_x_pos; x <= max_x_pos; x++) {
			WallOrientation w = WallOrientation::InvalidWall;
			int wall_start, y;
			for (y = min_y_pos; y <= max_y_pos; y++) {
				bool is_air = !CheckPlayerCollisionCurrentRoom(x, y);
				bool left_is_air = !CheckPlayerCollisionCurrentRoom(x - 1, y);
				
				WallOrientation w_new;
				if (is_air == left_is_air) {
					w_new = InvalidWall;
				} else {
					w_new = is_air ? WallOrientation::Left : WallOrientation::Right;
				}

				if (w != w_new) {
					if (w != InvalidWall) {
						// Wall ends here
						int wall_end = y;
						
						if (!is_air || !left_is_air) {
							// Block off "gaps" in non-convex corners
							wall_end += 1;
						}

						int wall_y = (w == WallOrientation::Left) ? wall_end : wall_start;
						int width = wall_end - wall_start;
						
						nav_walls.emplace_back(w, x, wall_y, rx, ry, width);
					}

					if (w_new != InvalidWall) {
						// New wall starts here
						wall_start = y;

						if (wall_start > min_y_pos) {
							bool up_is_air = !CheckPlayerCollisionCurrentRoom(x, wall_start - 1);
							bool upleft_is_air = !CheckPlayerCollisionCurrentRoom(x - 1, wall_start - 1);

							// Block off "gaps" in non-convex corners
							if (!up_is_air || !upleft_is_air) {
								wall_start -= 1;
							}
						}
					}

					w = w_new;
				}
			}

			if (w != InvalidWall) {
				// Wall ends at screen edge
				int wall_y = (w == WallOrientation::Left) ? y : wall_start;
				int width = y - wall_start;

				nav_walls.emplace_back(w, x, wall_y, rx, ry, width);
			}
		}

		// Create horizontal walls
		for (int y = min_y_pos; y <= max_y_pos; y++) {
			WallOrientation w = WallOrientation::InvalidWall;
			int wall_start, x;
			for (x = min_x_pos; x <= max_x_pos; x++) {
				bool is_air = !CheckPlayerCollisionCurrentRoom(x, y);
				bool up_is_air = !CheckPlayerCollisionCurrentRoom(x, y - 1);

				WallOrientation w_new;
				if (is_air == up_is_air) {
					w_new = InvalidWall;
				} else {
					w_new = is_air ? WallOrientation::Down : WallOrientation::Up;
				}

				if (w != w_new) {
					if (w != InvalidWall) {
						// Wall ends here
						int wall_x = (w == WallOrientation::Down) ? x : wall_start;
						int width = x - wall_start;
						if (!is_air || !up_is_air) {
							// Block off "gaps" in non-convex corners
							width += 1;
							if (w == WallOrientation::Down) {
								wall_x += 1;
							}
						}

						nav_walls.emplace_back(w, wall_x, y, rx, ry, width);
					}

					if (w_new != InvalidWall) {
						// New wall starts here
						wall_start = x;

						if (wall_start > min_x_pos) {
							bool left_is_air = !CheckPlayerCollisionCurrentRoom(wall_start - 1, y);
							bool upleft_is_air = !CheckPlayerCollisionCurrentRoom(wall_start - 1, y - 1);

							// Block off "gaps" in non-convex corners
							if (!left_is_air || !upleft_is_air) {
								wall_start -= 1;
							}
						}
					}

					w = w_new;
				}
			}

			if (w != InvalidWall) {
				// Wall ends at screen edge
				int wall_x = (w == WallOrientation::Down) ? x : wall_start;
				int width = x - wall_start;

				nav_walls.emplace_back(w, wall_x, y, rx, ry, width);
			}
		}

		precomputed_rooms[rx][ry].state = RoomInfoState::Walls;
	}

	void PrecomputeNavigationGraph() {
		int num_corners = nav_corners.size();
		int num_walls = nav_walls.size();
		// Determine connectivity of convex corners
		for (int i = 0; i < num_corners; i++) {
			NavCorner& corner_i = nav_corners.at(i);
			if (!IsConvexCorner(corner_i.type)) {
				continue;
			}

			for (int j = 0; j < i; j++) {
				NavCorner& corner_j = nav_corners.at(j);
				if (!IsConvexCorner(corner_j.type)) {
					continue;
				}

				// Distance estimate:
				int d_rx = corner_j.room_x - corner_i.room_x;
				int d_ry = corner_j.room_y - corner_i.room_y;
				if (d_rx < -10) {
					d_rx += 20;
				}
				else if (d_rx > 10) {
					d_rx -= 20;
				}
				if (d_ry < -10) {
					d_ry += 20;
				}
				else if (d_ry > 10) {
					d_ry -= 20;
				}

				int dx = 320 * d_rx + (corner_j.x - corner_i.x);
				int dy = 240 * d_ry + (corner_j.y - corner_i.y);

				// Add edges to the corners
				if (TryConnectCorners(corner_i, corner_j)) {
					NavEdge e1; // From corner_i to corner_j
					e1.target = j; e1.dx = dx; e1.dy = dy; e1.active = false; e1.backwards = false,
					corner_i.edges.push_back(e1);
				}
				if (TryConnectCorners(corner_j, corner_i)) {
					NavEdge e2; // From corner_j to corner_i
					e2.target = i; e2.dx = -dx; e2.dy = -dy; e2.active = false; e2.backwards = false,
					corner_j.edges.push_back(e2);
				}
			}
		}

		// Set roomstates
		for (int rx = 0; rx < 20; rx++) {
			for (int ry = 0; ry < 20; ry++) {
				if (precomputed_rooms[rx][ry].state >= RoomInfoState::Walls) {
					precomputed_rooms[rx][ry].state = RoomInfoState::NavigationGraph;
				}
			}
		}

		// Do BFS from target position backwards to find "active" edges
		if (GetRoomState(goal_room) != RoomInfoState::NavigationGraph) {
			VVV_exit(1);
			return;
		}

		std::vector<int> queue;
		if (num_corners >= (1 << 16)) {
			VVV_exit(1);
			return;
		}
		for (int i = 0; i < num_corners; i++) {
			NavCorner& c = nav_corners.at(i);
			if (!IsConvexCorner(c.type)) {
				continue;
			}

			int d_rx = c.room_x - goal_room.rx;
			int d_ry = c.room_y - goal_room.ry;
			if (d_rx < -10) {
				d_rx += 20;
			}
			else if (d_rx > 10) {
				d_rx -= 20;
			}
			if (d_ry < -10) {
				d_ry += 20;
			}
			else if (d_ry > 10) {
				d_ry -= 20;
			}
			int d_x = 320 * d_rx + (c.x - goal_x);
			int d_y = 240 * d_ry + (c.y - goal_y);

			if (!IsDirectionCompatible(c, d_x, d_y)) {
				continue;
			}
			
			// Check corner is reachable
			if (!CrossRoomRayCast(goal_room.rx, goal_room.ry, goal_x, goal_y, d_x, d_y)) {
				continue;
			}

			int num_edges = c.edges.size();
			if (num_edges >= (1 << 16)) {
				VVV_exit(1);
				return;
			}
			for (int j = 0; j < num_edges; j++) {
				NavEdge& e = c.edges.at(j);
				if (e.active || e.backwards) {
					// Already added this edge, continue
					continue;
				}

				// Can't change axis directions without going through zero
				if (d_x > 0 && e.dx < 0 || d_x < 0  && e.dx > 0 || d_y > 0 && e.dy < 0 || d_y < 0 && e.dy > 0) {
					continue;
				}

				// Unify corner directions to BottomRight via mirroring of edge directions to make checks easier
				int u_dx = d_x;
				int u_dy = d_y;
				int u_edx = e.dx;
				int u_edy = e.dy;
				switch (c.type) {
					case NavCornerType::BottomRight:
						// Do nothing
						break;
					case NavCornerType::BottomLeft:
						// Mirror x
						u_dx = -u_dx;
						u_edx = -u_edx;
						break;
					case NavCornerType::TopLeft:
						// Mirror both
						u_dx = -u_dx;
						u_edx = -u_edx;
						u_dy = -u_dy;
						u_edy = -u_edy;
						break;
					case NavCornerType::TopRight:
						// Mirror y
						u_dy = -u_dy;
						u_edy = -u_edy;
						break;
					default:
						// Should be unreachable
						continue;
				}

				// Mirror along y=-x if edge is going backwards (i.e. down and left, to reduce necessary checks even more
				if ((u_dx < 0 && u_dy > 0) || (u_edx < 0 && u_edy > 0)) {
					int tmp = u_dx;
					u_dx = -u_dy;
					u_dy = -tmp;
					tmp = u_edx;
					u_edx = -u_edy;
					u_edy = -tmp;
				}

				// Angle must be greater than 180 degrees
				if (u_dx <= 0 || u_dy > 0 || u_edx < 0 || u_edy >= 0) {
					continue;
				}

				// Angle must be greater than 180 degrees -> slope of line must increase (but note that y axis is inverted here)
				// Therefore, u_edy / u_edx < u_dy / u_dx, which can be written as:
				if (u_edy * u_dx >= u_dy * u_edx) {
					continue;
				}

				// Now we can mark the edge as active and add it to the queue!
				e.active = true;
				int edge_id = (i << 16) | j;
				queue.push_back(edge_id);

				// Mark the backwards edge
				NavCorner& target = nav_corners.at(e.target);
				for (int k = 0; k < target.edges.size(); k++) {
					NavEdge& backwards = target.edges.at(k);
					if (backwards.target == i) {
						backwards.backwards = true;
						break;
					}
				}
			}
		}

		while (queue.size() > 0) {
			int edge_id = queue.back();
			queue.pop_back();

			int corner_idx = edge_id >> 16;
			int edge_idx = edge_id & 0xffff;

			NavCorner& corner = nav_corners.at(corner_idx);
			NavEdge& edge = corner.edges.at(edge_idx);

			int target_corner_idx = edge.target;
			NavCorner& target_corner = nav_corners.at(target_corner_idx);

			int num_edges = target_corner.edges.size();
			for (int i = 0; i < num_edges; i++) {
				NavEdge& next_edge = target_corner.edges.at(i);
				if (next_edge.active || next_edge.backwards) {
					// Edge has already been added
					continue;
				}

				if (!CanConnectEdges(target_corner, edge, next_edge)) {
					continue;
				}

				next_edge.active = true;
				int next_edge_id = (target_corner_idx << 16) | i;
				queue.push_back(next_edge_id);

				// Mark the backwards edge
				NavCorner& target = nav_corners.at(next_edge.target);
				for (int j = 0; j < target.edges.size(); j++) {
					NavEdge& backwards = target.edges.at(j);
					if (backwards.target == corner_idx) {
						backwards.backwards = true;
						break;
					}
				}
			}
		}
	}

	bool CanConnectEdges(NavCorner& corner, NavEdge& incoming_edge, NavEdge& outgoing_edge) {
		// assume(incoming_edge.target == corner)

		int u_dx = incoming_edge.dx;
		int u_dy = incoming_edge.dy;
		int u_edx = outgoing_edge.dx;
		int u_edy = outgoing_edge.dy;
		// Can't change axis directions without going through zero
		if (u_dx > 0 && u_edx < 0 || u_dx < 0 && u_edx > 0 || u_dy > 0 && u_edy < 0 || u_dy < 0 && u_edy > 0) {
			return false;
		}

		// Unify corner directions to BottomRight via mirroring of edge directions to make checks easier
		switch (corner.type) {
			case NavCornerType::BottomRight:
				// Do nothing
				break;
			case NavCornerType::BottomLeft:
				// Mirror x
				u_dx = -u_dx;
				u_edx = -u_edx;
				break;
			case NavCornerType::TopLeft:
				// Mirror both
				u_dx = -u_dx;
				u_edx = -u_edx;
				u_dy = -u_dy;
				u_edy = -u_edy;
				break;
			case NavCornerType::TopRight:
				// Mirror y
				u_dy = -u_dy;
				u_edy = -u_edy;
				break;
			default:
				// Should be unreachable
				return false;
		}

		// Mirror along TL-BR diagonal if edge is going backwards (i.e. down and left, to reduce necessary checks even more)
		if ((u_dx <= 0 && u_dy > 0) || (u_edx < 0 && u_edy >= 0)) {
			int tmp = u_dx;
			u_dx = u_dy;
			u_dy = tmp;
			tmp = u_edx;
			u_edx = u_edy;
			u_edy = tmp;
		}

		// Angle must be greater than 180 degrees
		if (u_dx <= 0 || u_dy > 0 || u_edx < 0 || u_edy >= 0) {
			return false;
		}

		// Angle must be greater than 180 degrees -> slope of line must increase (but note that y axis is inverted here)
		// Therefore, u_edy / u_edx > u_dy / u_dx, which can be written as:
		if ((-u_edy * u_dx) <= (-u_dy * u_edx)) {
			return false;
		}

		// Now we can mark the edge as active and add it to the queue!
		return true;
	}

	bool IsDirectionCompatible(NavCorner& corner, int dx, int dy) {
		switch (corner.type) {
			case NavCornerType::TopLeft:
			case NavCornerType::BottomRight:
				if (dx < 0 && dy < 0 || dx > 0 && dy > 0) {
					return false;
				}
				break;
			case NavCornerType::TopRight:
			case NavCornerType::BottomLeft:
				if (dx < 0 && dy > 0 || dx > 0 && dy < 0) {
					return false;
				}
				break;
			default:
				return false;
		}
		return true;
	}

	bool IsConvexCorner(NavCornerType type) {
		switch (type) {
			case NavCornerType::TopLeft:
			case NavCornerType::TopRight:
			case NavCornerType::BottomLeft:
			case NavCornerType::BottomRight:
				return true;
			default:
				return false;
		}
	}

	// TODO: deal with wrapping of map
	bool TryConnectCorners(NavCorner& source, NavCorner& target) {
		if (!IsConvexCorner(source.type) || !IsConvexCorner(target.type)) {
			return false;
		}

		int d_rx = target.room_x - source.room_x;
		int d_ry = target.room_y - source.room_y;
		if (d_rx < -10) {
			d_rx += 20;
		} else if (d_rx > 10) {
			d_rx -= 20;
		}
		if (d_ry < -10) {
			d_ry += 20;
		} else if (d_ry > 10) {
			d_ry -= 20;
		}

		// Estimate:
		int d_x = 320 * d_rx + (target.x - source.x);
		int d_y = 240 * d_ry + (target.y - source.y);
		// TODO: account for other room dimensions, warps, etc.
		if (!IsDirectionCompatible(source, d_x, d_y) || !IsDirectionCompatible(target, d_x, d_y)) {
			return false;
		}

		// Ray
		int o_rx = source.room_x;
		int o_ry = source.room_y;
		int o_x = source.x;
		int o_y = source.y;

		return CrossRoomRayCast(o_rx, o_ry, o_x, o_y, d_x, d_y);
	}

	bool CrossRoomRayCast(int o_rx, int o_ry, int o_x, int o_y, int d_x, int d_y) {
		// Raycast wall intersections
		int num_walls = nav_walls.size();
		bool any_intersections = false;
		while (true) {
			if (GetRoomState(RoomCoords(o_rx, o_ry)) == None) {
				any_intersections = true;
				break;
			}

			for (int i = 0; i < num_walls; i++) {
				Wall& wall = nav_walls.at(i);
				if (wall.room_x != o_rx || wall.room_y != o_ry) {
					// Not in same room
					continue;
				}

				if (wall.RayIntersect(o_x, o_y, d_x, d_y)) {
					any_intersections = true;
					break;
				}
			}

			if (any_intersections) {
				break;
			}

			// Find first intersection with screen edge
			int x_step = d_x;
			int y_step = d_y;
			float t_x = INFINITY;
			float t_y = INFINITY;
			if (d_x != 0) {
				int x_edge = d_x > 0 ? (precomputed_rooms[o_rx][o_ry].max_x_pos + 1) : (precomputed_rooms[o_rx][o_ry].min_x_pos - 1);
				x_step = x_edge - o_x;
				t_x = ((float)x_step) / ((float)d_x);
			}
			if (d_y != 0) {
				int y_edge = d_y > 0 ? (precomputed_rooms[o_rx][o_ry].max_y_pos + 1) : (precomputed_rooms[o_rx][o_ry].min_y_pos - 1);
				y_step = y_edge - o_y;
				t_y = ((float)y_step) / ((float)d_y);
			}

			if (std::abs(x_step) >= std::abs(d_x) && std::abs(y_step) >= std::abs(d_y)) {
				// We are already in final room
				break;
			}

			if (t_x < t_y) {
				// Follow x direction
				o_x += x_step;
				d_x -= x_step;
				if (o_x < precomputed_rooms[o_rx][o_ry].min_x_pos) {
					o_x += 320;
					o_rx -= 1;
				}
				else if (o_x > precomputed_rooms[o_rx][o_ry].max_x_pos) {
					o_x -= 320;
					o_rx += 1;
				}
			} else {
				// Follow y direction
				// TODO: what about rooms with 232 height?
				o_y += y_step;
				d_y -= y_step;
				if (o_y < precomputed_rooms[o_rx][o_ry].min_y_pos) {
					o_y += 240;
					o_ry -= 1;
				}
				else if (o_y > precomputed_rooms[o_rx][o_ry].max_y_pos) {
					o_y -= 240;
					o_ry += 1;
				}
			}
		}

		return !any_intersections;
	}

	bool CheckPlayerCollisionCurrentRoom(int x, int y) {
		const SDL_Rect temprect = {x + VIRIDIAN_CX, y + VIRIDIAN_CY, VIRIDIAN_W, VIRIDIAN_H};

		// Check walls
		if (collision_kind == CollisionKind::Walls || collision_kind == CollisionKind::WallsAndSpikes) {
			if (obj.checkwall(false, temprect)) {
				return true;
			}
		}
		// Check spikes
		if (collision_kind == CollisionKind::WallsAndSpikes) {
			for (size_t j = 0; j < obj.blocks.size(); j++)
			{
				if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect)) {
					return true;
				}
			}
		}
		return false;
	}

	void FullPrecomputation(RoomCoords start) {
		if (num_precomputed_rooms > 0) {
			// Should only be called once
			VVV_exit(1);
			return;
		}

		LoadRoom(start);

		std::vector<RoomCoords> queue;
		PrecomputeCurrentRoomDims();
		queue.push_back(start);

		while (queue.size() > 0) {
			RoomCoords current_room = queue.back();
			queue.pop_back();

			LoadRoom(current_room);
			PrecomputeCurrentRoomWalls();
			num_precomputed_rooms += 1;

			int cur_rx = current_room.rx;
			int cur_ry = current_room.ry;

			int min_x = precomputed_rooms[cur_rx][cur_ry].min_x_pos;
			int max_x = precomputed_rooms[cur_rx][cur_ry].max_x_pos;
			int min_y = precomputed_rooms[cur_rx][cur_ry].min_y_pos;
			int max_y = precomputed_rooms[cur_rx][cur_ry].max_y_pos;
			bool warpx = precomputed_rooms[cur_rx][cur_ry].warpx;
			bool warpy = precomputed_rooms[cur_rx][cur_ry].warpy;

			// Check reachability of neighbouring rooms
			bool up, down, left, right;
			up = down = left = right = false;
			if (!warpy) {
				for (int x = min_x; (!up || !down) && x <= max_x; x++) {
					if (!up && !CheckPlayerCollisionCurrentRoom(x, min_y)) {
						up = true;
					}
					if (!down && !CheckPlayerCollisionCurrentRoom(x, max_y)) {
						down = true;
					}
				}
			}
			if (!warpx) {
				for (int y = min_y; (!left || !right) && y <= max_y; y++) {
					if (!left && !CheckPlayerCollisionCurrentRoom(min_x, y)) {
						left = true;
					}
					if (!right && !CheckPlayerCollisionCurrentRoom(max_x, y)) {
						right = true;
					}
				}
			}

			// Add reachable neighbors to queue
			if (up) {
				RoomCoords room_above = current_room.next_above();
				if (GetRoomState(room_above) == RoomInfoState::None) {
					LoadRoom(room_above);
					PrecomputeCurrentRoomDims();
					queue.push_back(room_above);
				}
			}
			if (down) {
				RoomCoords room_below = current_room.next_below();
				if (GetRoomState(room_below) == RoomInfoState::None) {
					LoadRoom(room_below);
					PrecomputeCurrentRoomDims();
					queue.push_back(room_below);
				}
			}
			if (left) {
				RoomCoords room_left = current_room.next_left();
				if (GetRoomState(room_left) == RoomInfoState::None) {
					LoadRoom(room_left);
					PrecomputeCurrentRoomDims();
					queue.push_back(room_left);
				}
			}
			if (right) {
				RoomCoords room_right = current_room.next_right();
				if (GetRoomState(room_right) == RoomInfoState::None) {
					LoadRoom(room_right);
					PrecomputeCurrentRoomDims();
					queue.push_back(room_right);
				}
			}
		}

		// Compute navigation graph for all rooms reached
		PrecomputeNavigationGraph();
	}

	void Precompute(void) {
		RoomCoords cur = GetCurrentRoomCoords();
		int rx = cur.rx;
		int ry = cur.ry;

		if (GetRoomState(cur) < RoomInfoState::Walls) {
			FullPrecomputation(cur);
			ResetState();
		}
	}
	
	// ------------------
	// Hook functions
	// ------------------

	void BeforeRenderHook(void) {
		Precompute();
	}

	void AfterTileRenderHook(void) {
		RoomCoords cur = GetCurrentRoomCoords();
		if (GetRoomState(cur) >= RoomInfoState::Walls) {
			int px = obj.entities[0].xp;
			int py = obj.entities[0].yp;
			int num_walls = nav_walls.size();
			int num_corners = nav_corners.size();

			graphics.set_blendmode(SDL_BLENDMODE_BLEND);

			// Draw walls
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 155, 0, 255);
			for (int i = 0; i < num_walls; i++) {
				Wall& w = nav_walls.at(i);
				RenderWall(w);
			}

			// Draw corners
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 0, 255);
			for (int i = 0; i < num_corners; i++) {
				NavCorner& c = nav_corners.at(i);
				RenderCorner(c);
			}

			// Draw active connections
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 255, 150);
			for (int i = 0; i < num_corners; i++) {
				NavCorner& c = nav_corners.at(i);
				int num_edges = c.edges.size();
				for (int j = 0; j < num_edges; j++) {
					NavEdge& e = c.edges.at(j);

					if (e.active) {
						NavCorner& t = nav_corners.at(e.target);
						RenderEdge(c, e);
					}
				}
			}
		}
	}

	void AfterRenderHook(void) {
		int px = obj.entities[0].xp;
		int py = obj.entities[0].yp;
		graphics.set_blendmode(SDL_BLENDMODE_NONE);

		// Draw player pos
		SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 255, 255);
		RenderPixel(px, py);

		// Draw goal pos
		RoomCoords current_room = GetCurrentRoomCoords();
		if (current_room.rx == goal_room.rx && current_room.ry == goal_room.ry) {
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 255, 255, 255, 255);
			RenderPixel(goal_x, goal_y);
		}
	}

	// -------------------
	// Rendering Functions
	// -------------------

	void RenderWall(Wall& wall) {
		RoomCoords current_room = GetCurrentRoomCoords();
		if (wall.room_x != current_room.rx || wall.room_y != current_room.ry) {
			return;
		}

		int x, y, w, h;
		switch (wall.orientation) {
			default:
				// Invalid wall, skip
				return;
			case WallOrientation::Up:
				x = wall.x;
				y = wall.y;
				w = wall.width;
				h = 1;
				break;
			case WallOrientation::Right:
				x = wall.x;
				y = wall.y;
				w = 1;
				h = wall.width;
				break;
			case WallOrientation::Down:
				x = wall.x - wall.width;
				y = wall.y - 1;
				w = wall.width;
				h = 1;
				break;
			case WallOrientation::Left:
				x = wall.x - 1;
				y = wall.y - wall.width;
				w = 1;
				h = wall.width;
				break;
		}

		const SDL_Rect rect = { x + RENDER_OFFSET_X, y + RENDER_OFFSET_Y, w, h };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}

	void RenderCorner(NavCorner& c) {
		RoomCoords current_room = GetCurrentRoomCoords();
		if (c.room_x != current_room.rx || c.room_y != current_room.ry) {
			return;
		}

		int x1, y1;
		int x2, y2;
		int x3, y3;
		int w = 2;
		int h = 2;

		switch (c.type) {
			default:
				// Invalid corner, skip
				return;
			case NavCornerType::BottomRight:
				x1 = c.x - w;
				y1 = c.y - h;
				x2 = x1 - w;
				y2 = y1;
				x3 = x1;
				y3 = y1 - h;
				break;
			case NavCornerType::BottomLeft:
				x1 = c.x;
				y1 = c.y - h;
				x2 = x1 + w;
				y2 = y1;
				x3 = x1;
				y3 = y1 - h;
				break;
			case NavCornerType::TopLeft:
				x1 = c.x;
				y1 = c.y;
				x2 = x1 + w;
				y2 = y1;
				x3 = x1;
				y3 = y1 + h;
				break;
			case NavCornerType::TopRight:
				x1 = c.x - w;
				y1 = c.y;
				x2 = x1 - w;
				y2 = y1;
				x3 = x1;
				y3 = y1 + h;
				break;

			case NavCornerType::ConcaveTL:
				x1 = c.x;
				y1 = c.y;
				x2 = x1 - w;
				y2 = y1;
				x3 = x1;
				y3 = y1 - h;
				break;
			case NavCornerType::ConcaveTR:
				x1 = c.x - w;
				y1 = c.y;
				x2 = x1 + w;
				y2 = y1;
				x3 = x1;
				y3 = y1 - h;
				break;
			case NavCornerType::ConcaveBR:
				x1 = c.x - w;
				y1 = c.y - h;
				x2 = x1 + w;
				y2 = y1;
				x3 = x1;
				y3 = y1 + h;
				break;
			case NavCornerType::ConcaveBL:
				x1 = c.x;
				y1 = c.y - h;
				x2 = x1 - w;
				y2 = y1;
				x3 = x1;
				y3 = y1 + h;
				break;

			case NavCornerType::QuadTLBR:
			case NavCornerType::QuadBLTR:
				// Unimplemented
				VVV_exit(-1);
				return;
		}

		const SDL_Rect rect1 = { x1 + RENDER_OFFSET_X, y1 + RENDER_OFFSET_Y, w, h };
		const SDL_Rect rect2 = { x2 + RENDER_OFFSET_X, y2 + RENDER_OFFSET_Y, w, h };
		const SDL_Rect rect3 = { x3 + RENDER_OFFSET_X, y3 + RENDER_OFFSET_Y, w, h };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect1);
		SDL_RenderFillRect(gameScreen.m_renderer, &rect2);
		SDL_RenderFillRect(gameScreen.m_renderer, &rect3);
	}

	void RenderEdge(NavCorner& c, NavEdge& edge) {
		RoomCoords current_room = GetCurrentRoomCoords();
		int rx = current_room.rx;
		int ry = current_room.ry;

		int d_rx = c.room_x - current_room.rx;
		int d_ry = c.room_y - current_room.ry;
		if (d_rx < -10) {
			d_rx += 20;
		}
		else if (d_rx > 10) {
			d_rx -= 20;
		}
		if (d_ry < -10) {
			d_ry += 20;
		}
		else if (d_ry > 10) {
			d_ry -= 20;
		}

		int o_x = 320 * d_rx + c.x;
		int o_y = 240 * d_ry + c.y;

		int d_x = edge.dx;
		int d_y = edge.dy;

		if (c.room_x != current_room.rx || c.room_y != current_room.ry) {
			// Basically an AABB check
			int min_x_pos = precomputed_rooms[rx][ry].min_x_pos;
			int max_x_pos = precomputed_rooms[rx][ry].max_x_pos;
			int min_y_pos = precomputed_rooms[rx][ry].min_y_pos;
			int max_y_pos = precomputed_rooms[rx][ry].max_y_pos;

			float t_min_x = ((float)(min_x_pos - o_x)) / ((float)d_x);
			float t_max_x = ((float)(max_x_pos - o_x)) / ((float)d_x);
			float t_min_y = ((float)(min_y_pos - o_y)) / ((float)d_y);
			float t_max_y = ((float)(max_y_pos - o_y)) / ((float)d_y);

			float t_min = std::max(t_min_x, t_min_y);
			float t_max = std::min(t_max_x, t_max_y);

			if (t_min > t_max) {
				// Ray doesn't touch room
				return;
			}
		}

		SDL_RenderDrawLine(gameScreen.m_renderer, o_x + RENDER_OFFSET_X, o_y + RENDER_OFFSET_Y, o_x + d_x + RENDER_OFFSET_X, o_y + d_y + RENDER_OFFSET_Y);
	}

	void RenderPixel(int x, int y) {
		const SDL_Rect rect = { x + RENDER_OFFSET_X, y + RENDER_OFFSET_Y, 1, 1 };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}

	// -------------------
	// Interface to VVVVVV
	// -------------------

	void ResetState() {
		Solver::load_scenario();
	}

	void LoadRoom(RoomCoords room_coords) {
		int rx = room_coords.rx % 20;
		int ry = room_coords.ry % 20;
		if (!map.finalmode) {
			rx += 100;
			ry += 100;
		}

		if (game.roomx != rx || game.roomy != ry) {
			map.gotoroom(rx, ry);
		}
	}

	RoomCoords GetCurrentRoomCoords(void) {
		int rx = game.roomx;
		int ry = game.roomy;

		if (!map.finalmode) {
			rx -= 100;
			ry -= 100;
		}
		else {
			// TODO:
			VVV_exit(-1);
		}

		return RoomCoords(rx, ry);
	}

	bool CheckSpike(int x, int y) {
		SDL_Rect temprect;
		temprect.x = x;
		temprect.y = y;
		temprect.w = 1;
		temprect.h = 1;

		for (size_t j = 0; j < obj.blocks.size(); j++)
		{
			if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect))
			{
				return true;
			}
		}
		return false;
	}

	bool CheckPlayerCollision(int x, int y) {
		SDL_Rect temprect;
		temprect.x = x;
		temprect.y = y;
		temprect.w = 12;
		temprect.h = 21;

		return obj.checkwall(false, temprect);
	}

	bool CheckPlayerSpike(int x, int y) {
		SDL_Rect temprect;
		temprect.x = x;
		temprect.y = y;
		temprect.w = 12;
		temprect.h = 21;

		for (size_t j = 0; j < obj.blocks.size(); j++)
		{
			if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect))
			{
				return true;
			}
		}
		return false;
	}

	// --------------------------------------
	// More rigorously implemented stuff here
	// --------------------------------------
	float OverworldRaycast(Ray& r) {
		// Note: Any rooms that are traversed must be initialized
		int rx = r.origin.rx;
		int ry = r.origin.ry;
		int x = r.origin.x;
		int y = r.origin.y;

		int dx = r.direction.x;
		int dy = r.direction.y;

		bool changed_room = true;
		while (changed_room) {
			changed_room = false;

			// Do a raycast within the current room
			Ray room_ray = Ray(rx, ry, x, y, dx, dy);
			float t_room = RoomRaycast(room_ray, rx, ry);
			if (0 < t_room && t_room < INFINITY) {
				// If we found a collision, return it
				return t_room;
			}

			// Otherwise, keep going until the next room transition, if there is one
			int next_x_edge = (dx > 0) ? GetXMax(rx, ry) + 1 : GetXMin(rx, ry) - 1;
			int next_y_edge = (dy > 0) ? GetYMax(rx, ry) + 1 : GetYMin(rx, ry) - 1;

			int x_edge_dist = next_x_edge - x;
			int y_edge_dist = next_y_edge - y;

			float t_x;
			if (dx > 0 && dx >= x_edge_dist) {
				t_x = x_edge_dist / ((float)dx);
			} else if (dx < 0 && dx <= x_edge_dist) {
				t_x = x_edge_dist / ((float)dx);
			} else {
				t_x = INFINITY;
			}

			float t_y;
			if (dy > 0 && dy >= y_edge_dist) {
				t_y = y_edge_dist / ((float) dy);
			} else if (dy < 0 && dy <= y_edge_dist) {
				t_y = y_edge_dist / ((float) dy);
			} else {
				t_y = INFINITY;
			}

			// Check room change conditions
			if (t_x < t_y) {
				// x edge is closer
				if (dx > 0) {
					x -= 320;
					rx += 1;
				} else {
					x += 320;
					rx -= 1;
				}
				changed_room = true;
			} else if (t_y < INFINITY) {
				// y edge is closer
				if (dy > 0) {
					y -= GetWarpY(rx, ry) ? 232 : 240;
					ry += 1;
				} else {
					y += GetWarpY(rx, ry) ? 232 : 240;
					ry -= 1;
				}
				changed_room = true;
			}
		}

		return INFINITY;
	}

	float RoomRaycast(Ray& r, int rx, int ry) {


		return INFINITY;
	}

	// --------------------------------------
	// Getter Functions
	// --------------------------------------
	bool GetWarpX(int rx, int ry) {
		// TODO
		return false;
	}
	bool GetWarpY(int rx, int ry) {
		// TODO
		return false;
	}

	int GetXMin(int rx, int ry) {
		if (GetWarpX(rx, ry)) {
			return -9;
		} else {
			return -14;
		}
	}
	int GetXMax(int rx, int ry) {
		if (GetWarpX(rx, ry)) {
			return 310;
		} else {
			return 307;
		}
	}
	int GetYMin(int rx, int ry) {
		if (GetWarpY(rx, ry)) {
			return -11;
		}
		else {
			return -2;
		}
	}
	int GetYMax(int rx, int ry) {
		if (GetWarpY(rx, ry)) {
			return 226;
		}
		else {
			return 237;
		}
	}
}
