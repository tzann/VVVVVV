#ifndef TERRAIN_H
#define TERRAIN_H

#include <cstddef>
#include <vector>

namespace Terrain {
	static int colors[12][3] = {
		{ 0xff, 0x00, 0x00 }, { 0xff, 0x7f, 0x00 }, { 0xff, 0xff, 0x00 }, { 0x7f, 0xff, 0x00 },
		{ 0x00, 0xff, 0x00 }, { 0x00, 0xff, 0x7f }, { 0x00, 0xff, 0xff }, { 0x00, 0x7f, 0xff },
		{ 0x00, 0x00, 0xff }, { 0x7f, 0x00, 0xff }, { 0xff, 0x00, 0xff }, { 0xff, 0x00, 0x7f },
	};

	void BeforeRenderHook(void);
	void AfterTileRenderHook(void);
	void AfterRenderHook(void);

	void RenderTile(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void RenderPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

	bool CheckWall(int x, int y);
	bool CheckSpike(int x, int y);
	bool CheckPlayerCollision(int x, int y);
	bool CheckPlayerSpike(int x, int y);

	struct Corner {
		int x;
		int y;
		int mask;

		Corner(int a, int b, int c) : x(a), y(b), mask(c) {}
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
}

#endif /* TERRAIN_H */
