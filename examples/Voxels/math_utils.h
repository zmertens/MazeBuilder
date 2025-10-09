#ifndef MATH_UTILS_H
#define MATH_UTILS_H

class MathUtils {
public:
    static int calculateChunkedCoordinate(float x, int chunk_size);
    static double getCurrentTime(int start_time, int start_ticks);
    static float calculateTimeOfDay(double current_time, int day_length);
    static float calculateDaylight(float time_of_day);
    static int calculateScaleFactor(SDL_Window* window);
    
    static void calculateSightVector(float rx, float ry, float* vx, float* vy, float* vz);
    static void calculateMotionVector(bool flying, int sz, int sx, float rx, float ry,
                                    float* vx, float* vy, float* vz);
    
    static int findHighestBlock(float x, float z, const std::vector<ChunkData>& chunks, int chunk_size);
    static bool checkCollision(int height, float* x, float* y, float* z, 
                              const ChunkData* chunk, int chunk_size);
    static bool playerIntersectsBlock(int height, float x, float y, float z, 
                                    int hx, int hy, int hz);
};

#endif // MATH_UTILS_H
