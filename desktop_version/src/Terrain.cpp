#include "Terrain.h"

#include <SDL.h>

#include "Graphics.h"
#include "Map.h"
#include "Entity.h"
#include "UtilityClass.h"
#include "Game.h"
#include "Exit.h"
#include "Screen.h"


namespace Terrain {
	bool precomputed_rooms[20][20] = { { false } };

	CollisionKind collision_kind = CollisionKind::Walls;
	std::vector<NavCorner> nav_corners;
	std::vector<Wall> nav_walls;

	void PrecomputeCurrentRoomTerrain(void) {
		int rx = game.roomx;
		int ry = game.roomy;
		int prev_num_corners = nav_corners.size();
		int prev_num_walls = nav_walls.size();

		// Min and max positions the player can stand at in this room
		int min_x_pos, min_y_pos, max_x_pos, max_y_pos;
		if (map.towermode) {
			VVV_exit(-1);
		} else {
			if (map.warpx) {
				min_x_pos = -9; max_x_pos = 310;
			} else {
				min_x_pos = -14; max_x_pos = 307;
			}
			if (map.warpy) {
				min_y_pos = -11; max_y_pos = 226;
			} else {
				min_y_pos = -2; max_y_pos = 237;
			}
		}

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
						int wall_y = (w == WallOrientation::Left) ? y : wall_start;
						int width = y - wall_start;
						
						nav_walls.emplace_back(w, x, wall_y, rx, ry, width);
					}

					if (w_new != InvalidWall) {
						// New wall starts here
						wall_start = y;
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

						nav_walls.emplace_back(w, wall_x, y, rx, ry, width);
					}

					if (w_new != InvalidWall) {
						// New wall starts here
						wall_start = x;
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

		int num_corners = nav_corners.size();
		int num_walls = nav_walls.size();
		// Determine connectivity of convex corners
		for (int i = prev_num_corners; i < num_corners; i++) {
			NavCorner& new_corner = nav_corners.at(i);
			
			switch (new_corner.type) {
				case NavCornerType::TopLeft:
				case NavCornerType::TopRight:
				case NavCornerType::BottomLeft:
				case NavCornerType::BottomRight:
					break;
				default:
					// Skip convex corners
					continue;
			}

			for (int j = 0; j < i; j++) {
				NavCorner& c = nav_corners.at(j);

				// Not in same room, skip
				if (c.room_x != new_corner.room_x || c.room_y != new_corner.room_y) {
					continue;
				}

				int dx = new_corner.x - c.x;
				int dy = new_corner.y - c.y;

				// Check directionality
				switch (new_corner.type) {
					case NavCornerType::TopLeft:
					case NavCornerType::BottomRight:
						if (dx < 0 && dy < 0 || dx > 0 && dy > 0) {
							continue;
						}
						break;
					case NavCornerType::TopRight:
					case NavCornerType::BottomLeft:
						if (dx < 0 && dy > 0 || dx > 0 && dy < 0) {
							continue;
						}
						break;
					default:
						// Skip convex corners
						continue;
				}
				switch (c.type) {
					case NavCornerType::TopLeft:
					case NavCornerType::BottomRight:
						if (dx < 0 && dy < 0 || dx > 0 && dy > 0) {
							continue;
						}
						break;
					case NavCornerType::TopRight:
					case NavCornerType::BottomLeft:
						if (dx < 0 && dy > 0 || dx > 0 && dy < 0) {
							continue;
						}
						break;
					default:
						// Skip convex corners
						continue;
				}

				// Add edges to the corners
				NavEdge e1, e2;
				e1.target = i; e1.dx = dx; e1.dy = dy;
				e2.target = j; e2.dx = -dx; e2.dy = -dy;

				c.edges.push_back(e1);
				new_corner.edges.push_back(e2);
			}
		}
	}

	bool CheckPlayerCollisionCurrentRoom(int x, int y) {
		const SDL_Rect temprect = {x + VIRIDIAN_CX, y + VIRIDIAN_CY, VIRIDIAN_W, VIRIDIAN_H};

		// Check walls
		if (collision_kind == Walls || collision_kind == WallsAndSpikes) {
			if (obj.checkwall(false, temprect)) {
				return true;
			}
		}
		// Check spikes
		if (collision_kind == WallsAndSpikes) {
			for (size_t j = 0; j < obj.blocks.size(); j++)
			{
				if (obj.blocks[j].type == DAMAGE && help.intersects(obj.blocks[j].rect, temprect)) {
					return true;
				}
			}
		}
		return false;
	}

	void Precompute(void) {
		int rx = game.roomx;
		int ry = game.roomy;

		if (!precomputed_rooms[rx][ry]) {
			PrecomputeCurrentRoomTerrain();
			precomputed_rooms[rx][ry] = true;
		}
	}

	void BeforeRenderHook(void) {
		Precompute();
	}

	void AfterTileRenderHook(void) {
		int rx = game.roomx;
		int ry = game.roomy;
		if (precomputed_rooms[rx][ry]) {
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

			// Draw connections
			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 255, 100);
			for (int i = 0; i < num_corners; i++) {
				NavCorner& c = nav_corners.at(i);
				int num_edges = c.edges.size();
				for (int j = 0; j < num_edges; j++) {
					NavEdge& e = c.edges.at(j);
					// Draw each edge only once
					if (i < e.target) {
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
	}

	void RenderWall(Wall& wall) {
		if (wall.room_x != game.roomx || wall.room_y != game.roomy) {
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
		if (c.room_x != game.roomx || c.room_y != game.roomy) {
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
		if (game.roomx != c.room_x || game.roomy != c.room_y) {
			return;
		}
		SDL_RenderDrawLine(gameScreen.m_renderer, c.x + RENDER_OFFSET_X, c.y + RENDER_OFFSET_Y, c.x + edge.dx + RENDER_OFFSET_X, c.y + edge.dy + RENDER_OFFSET_Y);
	}

	void RenderPixel(int x, int y) {
		const SDL_Rect rect = { x + RENDER_OFFSET_X, y + RENDER_OFFSET_Y, 1, 1 };
		SDL_RenderFillRect(gameScreen.m_renderer, &rect);
	}

	bool CheckWall(int x, int y) {
		return map.collide_precomputed(x / 8, y / 8);
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
}
