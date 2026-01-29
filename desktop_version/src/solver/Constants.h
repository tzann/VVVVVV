#ifndef SOLVER_CONSTANTS_H
#define SOLVER_CONSTANTS_H

#include "solver/Geometry.h"

#define REGULAR_FRAME_DELAY (34)

#define ROOM_W (320)
#define ROOM_H (240)

#define VIRIDIAN_CX (6)
#define VIRIDIAN_CY (2)
#define VIRIDIAN_W (12)
#define VIRIDIAN_H (21)

#define X_ACCEL_EFF (1.9f)
#define Y_ACCEL_EFF (2.75f)
#define MAX_X_SPEED (6)
#define MAX_Y_SPEED (10)
#define MAX_VX (6)
#define MAX_VY (10)

#define VX_INT_RANGE (IntInterval(-MAX_VX, MAX_VX))
#define FULL_X_SPEED_RANGE (FloatInterval(-MAX_X_SPEED, MAX_X_SPEED))
#define POS_X_SPEED_RANGE (FloatInterval(0.0f, MAX_X_SPEED))
#define NEG_X_SPEED_RANGE (FloatInterval(-MAX_X_SPEED, 0.0f))

#define VY_INT_RANGE (IntInterval(-MAX_VY, MAX_VY))
#define FULL_Y_SPEED_RANGE (FloatInterval(-MAX_Y_SPEED, MAX_Y_SPEED))
#define POS_Y_SPEED_RANGE (FloatInterval(0.0f, MAX_Y_SPEED))
#define NEG_Y_SPEED_RANGE (FloatInterval(-MAX_Y_SPEED, 0.0f))
#define Y_SPEED_RANGE_FOR_GRAVITY(inverseGravity) (inverseGravity ? NEG_Y_SPEED_RANGE : POS_Y_SPEED_RANGE)

#define PLATFORM_Y_SPEED_RANGE_FOR_GRAVITY(inverseGravity) (inverseGravity ? 0.0f : FloatInterval(0.0f, 0.75f))

#define MAX_SPEED_VECTOR (IntVector(MAX_X_SPEED, MAX_Y_SPEED));

#define X_RATE (1.1f)
#define Y_RATE (0.25f)

#define TOWER_RX (9)

#define RENDER_OFFSET_X (11)
#define RENDER_OFFSET_Y (12)

// Note that we touch the warp token / trinket in the range x [-17, 9], y [-22, 13]
#define TRINKET_X_MIN (-17)
#define TRINKET_X_MAX (9)
#define TRINKET_Y_MIN (-22)
#define TRINKET_Y_MAX (13)

#endif /* SOLVER_CONSTANTS_H */