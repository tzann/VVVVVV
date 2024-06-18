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
	bool is_precomputed = false;
	int precomputed_rx = 0;
	int precomputed_ry = 0;

	uint8_t pixels[320][240] = { { 0 } };

	std::vector<AABB> walls;
	std::vector<Corner> inside_corners;
	int *reachability;

	void AddWall(int c_x, int c_y, int c_mask, int last_x, int last_y, int last_mask) {
		// Give the walls some width
		if (c_x == last_x) {
			// Vertical wall
			switch (c_mask) {
			case 2: case 8: case 11: case 14:
				// Wall is to the right
				last_x += 1;
				break;
			case 1: case 4: case 7: case 13:
				// Wall is to the left
				last_x -= 1;
				break;
			}
			// Give concave corners extra wall to absorb rays exactly in the corner
			switch (c_mask) {
			case 1: case 2: case 4: case 8:
				if (c_y > last_y) {
					c_y += 2;
				}
				else if (c_y < last_y) {
					c_y -= 2;
				}
				break;
			}
			switch (last_mask) {
			case 1: case 2: case 4: case 8:
				if (c_y > last_y) {
					last_y -= 2;
				}
				else if (c_y < last_y) {
					last_y += 2;
				}
				break;
			}
		}
		else if (c_y == last_y) {
			// Horizontal wall
			switch (c_mask) {
			case 4: case 8: case 13: case 14:
				// Wall is above
				last_y -= 1;
				break;
			case 1: case 2: case 7: case 11:
				// Wall is below
				last_y += 1;
				break;
			}
			// Give concave corners extra wall to absorb rays exactly in the corner
			switch (c_mask) {
			case 1: case 2: case 4: case 8:
				if (c_x > last_x) {
					c_x += 2;
				}
				else if (c_x < last_x) {
					c_x -= 2;
				}
				break;
			}
			switch (last_mask) {
			case 1: case 2: case 4: case 8:
				if (c_x > last_x) {
					last_x -= 2;
				}
				else if (c_x < last_x) {
					last_x += 2;
				}
				break;
			}
		}
		else {
			VVV_exit(-1);
		}

		int min_x = last_x < c_x ? last_x : c_x;
		int max_x = last_x > c_x ? last_x : c_x;
		int min_y = last_y < c_y ? last_y : c_y;
		int max_y = last_y > c_y ? last_y : c_y;

		for (int x = min_x < 0 ? 0 : min_x; x < max_x && x < 320; x++) {
			for (int y = min_y < 0 ? 0 : min_y; y < max_y && y < 240; y++) {
				pixels[x][y] = 1;
			}
		}

		walls.emplace_back(min_x, max_x, min_y, max_y);
	}

	void Precompute(void) {
		if (reachability != NULL) {
			free(reachability);
		}
		walls.clear();
		inside_corners.clear();
		std::vector<Corner> corners;
		// Find all corner pixels
		for (int x = 0; x < 320; x++) {
			for (int y = 0; y < 240; y++) {
				bool is_air = !CheckPlayerCollision(x, y) && !CheckPlayerSpike(x, y);
				pixels[x][y] = is_air ? 2 : 0;

				int mask = 0;
				for (int bit = 1; bit < 16; bit <<= 1) {
					int _x = (bit == 2 || bit == 8) ? (x - 1) : x;
					int _y = (bit < 4) ? (y - 1) : y;
					bool _is_air = !CheckPlayerCollision(_x, _y) && !CheckPlayerSpike(_x, _y);
					if (_x < 0 || _y < 0 || _x == 319 || _y == 239) {
						// Treat offscreen as wall, but remember that it's a special case
						_is_air = false;
						mask |= 16;
					}

					if (_is_air) {
						mask |= bit;
					}
				}

				int mask_ignore_screen_edge = mask & 0b1111;
				switch (mask_ignore_screen_edge) {
					case 7: case 11: case 13: case 14:
						inside_corners.emplace_back(x, y, mask);
					case 1: case 2: case 4: case 8:
						corners.emplace_back(x, y, mask);
					default:
						break;
				}
			}
		}

		// For all corner pixels: Create a list, map out edges between them
		std::vector<Corner> sorted_corners;
		// Take last corner
		Corner& last = corners.back();
		sorted_corners.emplace_back(last.x, last.y, last.mask);
		corners.pop_back();

		int last_dir = -1; // 0 -> up, 1 -> right, 2 -> down, 3 -> left
		while (corners.size() > 0) {
			Corner &cur = sorted_corners.back();

			int h_dir, v_dir;
			switch (cur.mask & 0b1111) {
				case 1: case 14:
					h_dir = 1;
					v_dir = 0;
					break;
				case 2: case 13:
					h_dir = 3;
					v_dir = 0;
					break;
				case 4: case 11:
					h_dir = 1;
					v_dir = 2;
					break;
				case 8: case 7:
					h_dir = 3;
					v_dir = 2;
					break;
				default:
					h_dir = -1;
					v_dir = -1;
					VVV_exit(-1);
					break;
			}

			int desired_dir = h_dir;
			if (last_dir == desired_dir) {
				desired_dir = v_dir;
			}
			last_dir = (desired_dir + 2) % 4; // Update last_dir for next iteration

			int next_idx = -1;
			int closest_dist = 1000;
			for (int i = 0; i < corners.size(); i++) {
				const Corner& next = corners.at(i);

				int m = next.mask & 0b1111;

				bool same_axis = false;
				bool right_dir = false;
				bool good_mask = false;
				if (desired_dir % 2 == 0) {
					// x must be same
					same_axis = cur.x == next.x;
					if (desired_dir == 0) {
						// up
						right_dir = cur.y > next.y;
						good_mask = (m == 4 || m == 8 || m == 7 || m == 11);
					}
					else {
						// down
						right_dir = cur.y < next.y;
						good_mask = (m == 1 || m == 2 || m == 13 || m == 14);
					}
				}
				else {
					// y must be same
					same_axis = cur.y == next.y;
					if (desired_dir == 1) {
						// right
						right_dir = cur.x < next.x;
						good_mask = (m == 2 || m == 8 || m == 7 || m == 13);
					}
					else {
						// left
						right_dir = cur.x > next.x;
						good_mask = (m == 1 || m == 4 || m == 11 || m == 14);
					}
				}

				if (same_axis && right_dir && good_mask) {
					// We found a candidate corner
					int dist = abs(cur.x - next.x) + abs(cur.y - next.y);
					if (dist < closest_dist) {
						// Best one so far
						next_idx = i;
						closest_dist = dist;
					}
				}
			}

			if (next_idx != -1) {
				Corner& next = corners.at(next_idx);
				sorted_corners.emplace_back(next.x, next.y, next.mask);
				// Swap remove
				Corner& last_tmp = corners.back();
				corners[next_idx] = last_tmp;
				corners.pop_back();
			}
			else {
				// Start a new cycle
				sorted_corners.emplace_back(-1, -1, -1);
				// Take last corner
				Corner& last = corners.back();
				sorted_corners.emplace_back(last.x, last.y, last.mask);
				corners.pop_back();
			}
		}

		int last_x = -1;
		int last_y = -1;
		int last_mask = -1;

		int start_x = -1;
		int start_y = -1;
		int start_mask = -1;
		for (int i = 0; i < sorted_corners.size(); i++) {
			Corner& c = sorted_corners[i];

			if (last_x == -1) {
				// new cycle
				start_x = c.x;
				start_y = c.y;
				start_mask = c.mask;
				last_x = c.x;
				last_y = c.y;
				last_mask = c.mask;
				continue;
			}
			else if (c.mask == -1) {
				// end of cycle, potentially draw edge
				if ((last_mask & start_mask & 0b10000) == 0) {
					AddWall(start_x, start_y, start_mask & 0b1111, last_x, last_y, last_mask & 0b1111);
				}
				last_x = -1;
				start_x = -1;
				continue;
			}

			// Draw edge if it's not a screen edge
			if ((last_mask & c.mask & 0b10000) == 0) {
				AddWall(c.x, c.y, c.mask & 0b1111, last_x, last_y, last_mask & 0b1111);
			}
			last_x = c.x;
			last_y = c.y;
			last_mask = c.mask;
		}

		// Draw final edge if it's not a screen edge
		if (last_x != -1 && start_x != -1 && (last_mask & start_mask & 0b10000) == 0) {
			AddWall(start_x, start_y, start_mask & 0b1111, last_x, last_y, last_mask & 0b1111);
		}

		int num_corners = inside_corners.size();
		reachability = (int*) malloc(num_corners * num_corners * sizeof(int));
		// Determine reachability / distance between convex corners
		for (int i = 0; i < inside_corners.size(); i++) {
			Corner& c1 = inside_corners.at(i);

			for (int j = i + 1; j < inside_corners.size(); j++) {
				Corner& c2 = inside_corners.at(j);
				int dx = c2.x - c1.x;
				int dy = c2.y - c1.y;
				int c1_mask = c1.mask & 0b1111;
				int c2_mask = c2.mask & 0b1111;

				bool any = false;
				// Make sure the corners are compatible
				if (dx == 0) {
					if (dy > 0) {
						if (c1_mask != 7 && c1_mask != 11) {
							any = true;
						} else if (c1_mask == 7 && c2_mask != 13) {
							any = true;
						}
						else if (c1_mask == 11 && c2_mask != 14) {
							any = true;
						}
					}
					else {
						if (c2_mask != 7 && c2_mask != 11) {
							any = true;
						}
						else if (c2_mask == 7 && c1_mask != 13) {
							any = true;
						}
						else if (c2_mask == 11 && c1_mask != 14) {
							any = true;
						}
					}
				}
				else if (dy == 0) {
					if (dx > 0) {
						if (c1_mask != 14 && c1_mask != 11) {
							any = true;
						} else if (c1_mask == 14 && c2_mask != 13) {
							any = true;
						}
						else if (c1_mask == 11 && c2_mask != 7) {
							any = true;
						}
					}
					else {
						if (c2_mask != 14 && c2_mask != 11) {
							any = true;
						} if (c2_mask == 14 && c1_mask != 13) {
							any = true;
						}
						else if (c2_mask == 11 && c1_mask != 7) {
							any = true;
						}
					}
				}
				else if (c1_mask == c2_mask) {
					if ((dx > 0) == (dy > 0)) {
						if (c1_mask == 11 || c1_mask == 13) {
							any = true;
						}
					}
					else {
						if (c1_mask == 7 || c1_mask == 14) {
							any = true;
						}
					}
				}
				else if ((c1_mask & c2_mask) == 6 || (c1_mask & c2_mask) == 9) {
					// Corners are opposite -> same "illegal" areas
					if ((dx > 0) == (dy > 0)) {
						if (c1_mask == 11 || c1_mask == 13) {
							any = true;
						}
					}
					else {
						if (c1_mask == 7 || c1_mask == 14) {
							any = true;
						}
					}
				}
				else {
					any = true;
				}

				for (int w = 0; !any && w < walls.size(); w++) {
					AABB& wall = walls.at(w);
					if (wall.RayIntersect(c1.x, c1.y, dx, dy)) {
						any = true;
					}
				}
				if (!any) {
					reachability[i * num_corners + j] = 1;
					reachability[j * num_corners + i] = 1;
				}
				else {
					reachability[i * num_corners + j] = -1;
					reachability[j * num_corners + i] = -1;
				}
			}

			reachability[i * num_corners + i] = 0;
		}

		// Compute transitive closure of reachability
		bool changed = true;
		while (changed) {
			changed = false;
			for (int i = 0; i < num_corners; i++) {
				for (int j = 0; j < num_corners; j++) {
					int ij = reachability[i * num_corners + j];
					if (ij <= 0) {
						continue;
					}
					for (int k = 0; k < num_corners; k++) {
						int jk = reachability[j * num_corners + k];
						if (jk <= 0) {
							continue;
						}

						int ik = reachability[i * num_corners + k];
						if (ik == -1 || ik > ij + jk) {
							reachability[i * num_corners + k] = ij + jk;
							changed = true;
						}
					}
				}
			}
		}

		is_precomputed = true;
		precomputed_rx = game.roomx;
		precomputed_ry = game.roomy;
	}

	void BeforeRenderHook(void) {
		if (is_precomputed) {
			if (game.roomx != precomputed_rx || game.roomy != precomputed_ry) {
				is_precomputed = false;
			}
		}

		if (!is_precomputed) {
			Precompute();
		}
	}

	void AfterTileRenderHook(void) {
		if (is_precomputed) {
			int px = obj.entities[0].xp + 6;
			int py = obj.entities[0].yp + 2;
			int num_corners = inside_corners.size();

			graphics.set_blendmode(SDL_BLENDMODE_BLEND);

			// First, raycast from all corners to player to find distances
			int *corner_dists = (int*)malloc(num_corners * sizeof(int));
			for (int i = 0; i < num_corners; i++) {
				corner_dists[i] = -1; // Overwrite values
			}
			for (int i = 0; i < num_corners; i++) {
				Corner& c = inside_corners.at(i);
				int dx = c.x - px;
				int dy = c.y - py;
				// Make sure we're in the right place relative to the corner
				if ((dx > 0) != (dy > 0)) {
					if (c.mask == 7 || c.mask == 14) {
						continue;
					}
				}
				else {
					if (c.mask == 11 || c.mask == 13) {
						continue;
					}
				}

				bool any = false;
				for (int w = 0; w < walls.size(); w++) {
					AABB& wall = walls.at(w);
					if (wall.RayIntersect(px, py, dx, dy)) {
						any = true;
						break;
					}
				}
				if (!any) {
					corner_dists[i] = 0;

					// "Transitive closure"
					for (int j = 0; j < num_corners; j++) {
						if (j != i && reachability[i * num_corners + j] != -1) {
							if (corner_dists[j] == -1 || corner_dists[j] > reachability[i * num_corners + j]) {
								corner_dists[j] = reachability[i * num_corners + j];
							}
						}
					}
				}
			}
			
			for (int x = 0; x < 320; x++) {
				for (int y = 0; y < 240; y++) {
					if (pixels[x][y] == 1) {
						// Wall pixels
						Terrain::RenderPixel(x, y, 0, 255, 0, 127);
					}
					else if (pixels[x][y] == 2) {
						// Air pixels
						int d = num_corners + 1;
						// Raycast to player
						int dx = x - px;
						int dy = y - py;
						bool any = false;
						for (int w = 0; w < walls.size(); w++) {
							AABB& wall = walls.at(w);
							if (wall.RayIntersect(px, py, dx, dy)) {
								any = true;
								break;
							}
						}
						if (!any) {
							d = 0;
						}
						else {
							for (int i = 0; i < num_corners; i++) {
								Corner& c = inside_corners.at(i);
								if (corner_dists[i] >= d || corner_dists[i] == -1) {
									continue;
								}

								// Raycast to corner
								int dx = x - c.x;
								int dy = y - c.y;
								// Make sure we're in the right place relative to the corner
								if (dx != 0 && dy != 0) {
									if ((dx > 0) != (dy > 0)) {
										if (c.mask == 7 || c.mask == 14) {
											continue;
										}
									}
									else {
										if (c.mask == 11 || c.mask == 13) {
											continue;
										}
									}
								}
								bool any = false;
								for (int w = 0; w < walls.size(); w++) {
									AABB& wall = walls.at(w);
									if (wall.RayIntersect(c.x, c.y, dx, dy)) {
										any = true;
										break;
									}
								}
								if (!any) {
									int dist = 1 + corner_dists[i];
									if (dist < d) {
										d = dist;
									}
								}
							}
						}

						if (d <= num_corners) {
							if (d > 2) {
								d += 1;
							}
							d %= 12;
							Terrain::RenderPixel(x, y, colors[d][0], colors[d][1], colors[d][2], 127);
						}
					}
				}
			}

			SDL_SetRenderDrawColor(gameScreen.m_renderer, 0, 255, 255, 200);
			for (int i = 0; i < num_corners; i++) {
				Corner& ci = inside_corners.at(i);
				for (int j = i+1; j < num_corners; j++) {
					if (reachability[i * num_corners + j] == 1) {
						Corner& cj = inside_corners.at(j);

						SDL_RenderDrawLine(gameScreen.m_renderer, ci.x, ci.y, cj.x, cj.y);
					}
				}

				if (corner_dists[i] == 0) {
					SDL_RenderDrawLine(gameScreen.m_renderer, ci.x, ci.y, px, py);
				}
			}

			graphics.set_blendmode(SDL_BLENDMODE_NONE);
			Terrain::RenderPixel(px, py, 255, 255, 255, 255);
		}
	}

	void AfterRenderHook(void) {

	}

	void RenderTile(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
		graphics.fill_rect(8*x, 8*y, 8, 8, r, g, b, a);
	}

	void RenderPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
		graphics.fill_rect(x, y, 1, 1, r, g, b, a);
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
