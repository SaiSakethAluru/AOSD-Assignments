#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

#define PB2_SET_TYPE    _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT      _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO    _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT     _IOR(0x10, 0x34, int32_t*)



// simple test values
int32_t tmp;
int32_t val[10] = {9, 3, 6, 4, 2, 5, 2, 12, 15, 7};
int32_t minsorted[10] = {2, 2, 3, 4, 5, 6, 7, 9, 12, 15};
int32_t maxsorted[10] = {15, 12, 9, 7, 6, 5, 4, 3, 2, 2};

// large test case
int operations[1101] = {1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int32_t outputs[1101] = {-1, -1, -1, 9516, 9516, -1, -1, 6967, 6967, -1, -1, -1, 7837, 7837, -1, -1, -1, -1, -1, 7126, 7126, -1, -1, 53, 7104, 3024, 53, 5984, 53, 565, 53, 565, 5320, 8380, 5483, 3024, 5320, 6453, 5483, 3879, 5995, 3879, 5984, 8703, 5995, 7520, 6453, 7104, 2978, 2978, 4375, 2030, 5049, 2030, 3881, 3593, 3593, 3881, 1484, 1484, 4868, 4375, 4843, 4843, 6450, 9152, 7082, 4868, 5049, 6450, 7082, 975, 975, 4296, 4103, 4103, 32, 32, 4296, 5427, 5427, 6331, 9155, 8179, 6331, 7520, 3218, 2062, 2062, 3218, 1156, 1534, 1156, 2721, 7385, 1534, 2721, 7385, 8179, 3835, 3835, 671, 5863, 1810, 671, 8797, 2098, 1810, 8660, 7922, 2098, 5863, 3283, 1828, 1828, 7254, 3283, 7254, 1308, 9592, 9965, 1961, 1425, 1308, 6665, 1425, 5964, 6225, 652, 652, 9328, 1961, 9951, 7763, 5964, 6966, 4663, 4663, 6225, 6665, 2083, 2844, 2083, 2844, 6966, 7763, 7922, 7045, 7045, 5349, 5339, 5339, 6770, 5349, 6770, 6336, 6336, 5915, 231, 231, 7556, 2762, 241, 8359, 241, 1835, 1835, 1492, 1492, 2762, 6844, 5915, 324, 375, 324, 375, 7772, 5199, 5199, 546, 830, 546, 776, 776, 968, 830, 3903, 968, 3903, 6844, 7556, 7772, 8359, 8380, 2580, 931, 8625, 4858, 1095, 931, 787, 723, 1263, 9004, 723, 787, 8705, 9521, 5581, 3778, 1095, 3191, 1263, 2580, 3191, 3778, 5443, 4858, 4398, 4398, 5443, 3589, 9285, 2350, 2046, 2046, 8238, 1240, 1240, 7010, 2350, 3589, 3482, 8063, 3482, 6505, 6462, 9718, 5581, 4761, 8788, 8827, 4761, 4, 4, 6678, 6462, 6115, 6115, 5449, 7060, 4970, 3717, 3717, 4970, 5449, 6505, 6678, 7010, 7060, 3482, 3482, 8063, 939, 939, 6943, 9249, 6943, 8238, 8625, 240, 9420, 5062, 240, 9396, 5062, 8660, 8703, 4020, 1005, 9847, 4273, 389, 389, 1005, 1924, 1924, 8959, 4020, 4273, 8999, 9484, 8705, 53, 53, 1335, 7855, 8791, 1335, 4200, 7564, 4200, 7564, 2575, 2707, 2575, 459, 459, 4757, 2707, 4757, 2234, 2234, 7855, 9282, 8788, 8791, 8797, 3216, 3216, 8827, 8959, 8999, 2064, 2064, 3221, 3221, 9004, 9152, 9155, 9249, 1196, 1196, 9282, 9285, 9328, 9396, 9420, 9484, 3876, 7341, 5251, 3876, 5426, 5251, 7419, 4887, 2708, 3267, 2708, 3267, 4887, 1212, 1212, 5426, 7451, 6908, 6908, 3113, 1223, 8994, 1223, 6546, 3113, 6546, 7341, 555, 4368, 555, 4368, 7419, 4782, 4782, 7451, 8994, 739, 739, 6392, 1720, 1720, 6392, 9521, 7842, 950, 3080, 3822, 8103, 5182, 5199, 950, 3915, 3080, 3570, 3570, 2860, 5554, 1132, 9413, 1132, 1573, 862, 5364, 1402, 7072, 3800, 862, 1402, 4911, 4926, 1573, 831, 7481, 831, 8259, 4710, 1683, 1683, 2860, 3800, 3822, 4066, 3915, 4066, 4710, 4911, 4982, 4926, 4982, 5182, 5199, 5364, 18, 18, 5554, 7642, 5336, 5336, 7396, 7072, 186, 186, 7396, 7481, 7642, 5575, 5575, 1529, 1529, 3502, 3502, 7842, 8103, 8259, 9413, 9592, 8390, 8390, 4641, 4641, 7633, 2800, 9614, 9337, 2800, 2514, 9524, 8285, 5534, 2529, 2514, 8741, 2529, 5534, 7633, 8285, 8741, 3825, 9677, 9246, 9424, 9649, 3825, 2476, 2476, 9246, 6220, 5804, 5804, 4117, 4117, 3962, 5211, 3962, 9398, 5211, 2798, 2798, 6220, 5174, 4616, 4616, 9559, 5174, 9337, 6470, 8074, 8697, 8304, 6470, 3354, 3354, 9667, 8074, 8304, 8625, 6222, 9342, 2920, 7486, 2920, 5455, 6754, 5455, 5249, 8692, 3386, 3386, 8055, 4067, 4067, 5249, 6222, 6808, 4292, 4794, 1060, 1060, 9582, 4292, 4794, 1245, 1245, 5699, 5699, 9257, 6754, 2325, 2325, 6808, 7486, 7244, 5303, 5303, 7244, 8055, 9319, 2878, 8208, 2878, 8208, 8625, 8692, 8697, 9257, 2223, 2223, 2725, 2289, 2289, 2725, 456, 2458, 456, 2756, 2458, 2756, 8417, 8417, 9319, 9342, 9398, 3604, 3604, 9203, 6834, 6834, 9203, 2760, 1034, 4046, 1034, 1196, 9488, 2640, 5136, 8830, 7858, 2929, 2187, 1196, 2187, 2640, 2760, 2929, 4046, 4953, 4953, 7965, 5136, 7858, 6791, 6791, 7965, 9889, 4689, 4689, 8830, 2198, 4250, 2198, 4250, 9424, 5814, 5814, 6835, 6212, 6212, 6835, 8603, 879, 7018, 9317, 9607, 7218, 4200, 8663, 879, 3494, 3494, 4200, 7018, 8660, 7218, 5568, 5568, 8603, 1163, 1163, 8660, 8663, 9317, 7612, 7612, 9488, 9524, 9584, 6834, 564, 7401, 564, 9829, 6834, 7308, 7308, 7401, 9559, 9582, 9584, 9607, 9614, 2975, 5340, 2975, 2927, 2927, 1708, 1708, 1980, 1980, 4918, 4918, 8690, 5416, 5340, 2043, 9519, 2043, 276, 276, 5416, 4844, 6516, 4834, 5591, 4834, 4844, 5591, 6516, 8690, 5695, 5695, 9519, 9153, 6599, 6599, 9153, 9649, 9667, 9677, 1993, 1993, 9718, 4541, 4541, 8936, 5846, 6361, 5846, 6361, 8936, 9829, 9847, 1483, 5947, 1483, 7492, 5947, 4459, 9283, 2779, 2779, 7654, 3538, 2965, 2965, 9784, 3538, 4459, 7492, 7654, 1696, 1696, 9283, 9784, 9889, 9951, 9965, 56, 56, 4541, 4409, 4409, 2826, 7130, 6065, 6251, 7936, 3278, 2059, 5977, 2059, 3502, 2826, 3278, 3502, 4541, 4582, 4582, 1865, 5296, 7282, 1944, 385, 7891, 385, 7805, 1865, 8977, 1944, 8740, 5296, 5977, 6065, 6251, 8753, 7130, 7282, 7805, 7891, 7385, 7385, 7936, 1255, 1255, 8740, 8753, 1028, 1028, 4732, 4732, 6783, 9936, 6783, 8977, 6232, 6232, 9936, -1, 2651, 1412, 273, 3942, 540, 273, 4359, 9066, 5792, 1908, 1635, 540, 2265, 1412, 505, 505, 6083, 5442, 8419, 1635, 1908, 2265, 2168, 1964, 2732, 1964, 2231, 5347, 2168, 2231, 4102, 2501, 4087, 5193, 1404, 1404, 2501, 2651, 275, 275, 5036, 1940, 1940, 4110, 5596, 9606, 2732, 511, 511, 7047, 3942, 4087, 4102, 7730, 4110, 4981, 9577, 6305, 4359, 4981, 1983, 1983, 44, 44, 1463, 1463, 1364, 6216, 1364, 9173, 6091, 1749, 1749, 7879, 5036, 5679, 961, 4194, 961, 7015, 4194, 7312, 5193, 5347, 9187, 5442, 3755, 3755, 5735, 2162, 6815, 1400, 1400, 2162, 4778, 5679, 4778, 2142, 2142, 5596, 4447, 4447, 9879, 4642, 4642, 5679, 582, 582, 4171, 1258, 5761, 1258, 7090, 6512, 5432, 9972, 4171, 5432, 5679, 7526, 6283, 6901, 5735, 5761, 5792, 6083, 8804, 6091, 6216, 3034, 7276, 3034, 6283, 8091, 6305, 2068, 2068, 6096, 6811, 6096, 6512, 6811, 4698, 4698, 6815, 1417, 7326, 4165, 4739, 6664, 1417, 4165, 6086, 5191, 4739, 5031, 6613, 5031, 3293, 3293, 5448, 5191, 5448, 7075, 6086, 6613, 9922, 8494, 7703, 3299, 3299, 6664, 4272, 4272, 6901, 7015, 7047, 7075, 2802, 2802, 7090, 7422, 7276, 7312, 7555, 9947, 4654, 4654, 3096, 9930, 3096, 7326, 4605, 7125, 4507, 6716, 5234, 4771, 8440, 1683, 2273, 9141, 4223, 2890, 9794, 2075, 6992, 2930, 3798, 1422, 5695, 4279, 9969, 2087, 1452, 2170, 2669, 2940, 2748, 6584, 7757, 6407, 6202, 2462, 275, 2187, 2549, 5086, 9532, 8066, 3191, 6397, 2709, 2271, 6753, 8828, 5799, 5530, 5022, 1492, 6456, 3799, 1948, 1036, 8475, 1113, 5109, 6545, 8323, 4520, 7090, 3987, 6061, 4224, 9976, 5527, 3634, 3203, 7449, 8885, 4909, 7575, 9786, 9305, 1874, 797, 9095, 567, 9908, 577, 3334, 1253, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};	




struct pb2_set_type_arguments { 
    int32_t heap_size; // size of the heap
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
};
struct obj_info {
    int32_t heap_size; // current number of elements in heap (current size of the heap)
    int32_t heap_type; // heap type: 0 for min-heap, 1 for max-heap
    int32_t root; // value of the root node of the heap (null if size is 0).
    int32_t last_inserted; // value of the last element inserted in the heap.
};

struct result {
    int32_t result;         // value of min/max element extracted.
    int32_t heap_size;     // current size of the heap after extracting.
};

int checkStats(int fd, int heap_size, int heap_type, int root, int last_inserted){

    struct obj_info myobj_info;

    int result = ioctl(fd, PB2_GET_INFO, &myobj_info);
    if(myobj_info.heap_size == heap_size){
        printf(GRN "Size Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Size Do Not Match. Expected %d, Found %d\n" RESET, heap_size, (int)myobj_info.heap_size);
    }

    if(myobj_info.heap_type == heap_type){
        printf(GRN "Type Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! Type Do Not Match. Expected %d, Found %d\n" RESET, heap_type, (int)myobj_info.heap_type);
    }

    if(myobj_info.root == root){
        printf(GRN "ROOT Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! ROOT Do Not Match. Expected %d, Found %d\n" RESET, (int)root, (int)myobj_info.root);
    }

    if(myobj_info.last_inserted == last_inserted){
        printf(GRN "last_inserted Matched\n" RESET);
    }
    else {
        printf(RED "ERROR! last_inserted Do Not Match. Expected %d, Found %d\n" RESET, (int)last_inserted, (int)myobj_info.last_inserted);
    }
    return 0;
}

int basicMinHeapTest(int fd){

    struct pb2_set_type_arguments mypb2_set_type_arguments;
    struct obj_info myobj_info;
    struct result myresult;

    // Initialize min heap =============================================
    printf("==== Initializing Min Heap ====\n");
    char buf[2];
    // min heap
    mypb2_set_type_arguments.heap_type = 0;
    // size of the heap
    mypb2_set_type_arguments.heap_size = 20;

    int result = ioctl(fd, PB2_SET_TYPE, &mypb2_set_type_arguments);

    if(result < 0){
        perror("Initialization failed");
        close(fd);
        return 0;
    }
    printf("Written %d bytes\n", result);

    checkStats(fd, 0, 0, 0, 0);

    // Test Min Heap ====================================================
    printf("\n======= Test Min Heap =========\n");

    // Insert --------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Inserting %d: ", val[i]);
        result = ioctl(fd, PB2_INSERT, &val[i]);
        if(result < 0){
            perror("ERROR! Insert failed");
            close(fd);
            return 0;
        }
        printf("Written %d bytes. ", result);
    }
    checkStats(fd, 10, 0, 2, 7);
    

    printf("\n");
    
    // Verify ---------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Extracting.. ");
        result = ioctl(fd, PB2_EXTRACT, &myresult);
        if(result < 0){
            perror(RED "ERROR! Read failed\n" RESET);
            return 0;
        }
        printf("%d.", (int)tmp);
        if(myresult.result == minsorted[i]){
            printf(GRN "Match. " RESET);
        }
        else {
            printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)minsorted[i], (int)tmp);
        }
    }

    checkStats(fd, 0, 0, 0, 7);

    return 0;
}



int basicMaxHeapTest(int fd){

    struct pb2_set_type_arguments mypb2_set_type_arguments;
    struct obj_info myobj_info;
    struct result myresult;


    // Initialize min heap =============================================
    printf("==== Initializing Max Heap ====\n");
    char buf[2];
    // max heap
    mypb2_set_type_arguments.heap_type = 1;
    // size of the heap
    mypb2_set_type_arguments.heap_size = 20;

    int result = ioctl(fd, PB2_SET_TYPE, &mypb2_set_type_arguments);

    if(result < 0){
        perror("Initialization failed");
        close(fd);
        return 0;
    }
    printf("Written %d bytes\n", result);

    checkStats(fd, 0, 1, 0, 0);


    printf("\n======= Test Max Heap =========\n");
    // Insert --------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Inserting: %d,", val[i]);
        result = ioctl(fd, PB2_INSERT, &val[i]);
        if(result < 0){
            perror("ERROR! Write failed");
            return 0;
        }
        printf("Written %d bytes.", result);
    }
    checkStats(fd, 0, 1, 15, 7);

    printf("\n");
    // Verify ---------------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        printf("Extracting..");
        result = ioctl(fd, PB2_EXTRACT, &myresult);
        if(result < 0){
            perror("ERROR! Read failed");
            return 0;
        }
        printf("%d.", (int)tmp);
        if(myresult.result == maxsorted[i]){
            printf(GRN "Match. " RESET);
        }
        else {
            printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)maxsorted[i], (int)tmp);
        }
    }
    checkStats(fd, 0, 1, 0, 7);

    return 0;
}


int largeTest(int fd){
    printf("\n============= Large Test Set =====================\n");

    struct pb2_set_type_arguments mypb2_set_type_arguments;
    struct obj_info myobj_info;
    struct result myresult;


    // Initialize min heap =============================================
    printf("==== Initializing Min Heap ====\n");
    char buf[2];
    // min heap
    mypb2_set_type_arguments.heap_type = 0;
    // size of the heap
    mypb2_set_type_arguments.heap_size = 100;

    int result = ioctl(fd, PB2_SET_TYPE, &mypb2_set_type_arguments);

    if(result < 0){
        perror("Initialization failed");
        close(fd);
        return 0;
    }
    printf("Written %d bytes\n", result);

    checkStats(fd, 0, 0, 0, 0);



    // Test Heap ====================================================
    // printf("\n======= Test Min Heap =========\n");

    // Insert --------------------------------------------------------
    for (int i = 0; i < 1101; i++)
    {
        if(!operations[i]){
            // insert
            if(outputs[i] >= 0){
                printf("Inserting: %d,", outputs[i]);
                result = ioctl(fd, PB2_INSERT, &outputs[i]);

                if(result < 0){
                    perror(RED "ERROR! Insert failed\n" RESET);
                    // return 0;
                }
            }
            else {
                printf("Inserting: %d,", val[0]);
                result = ioctl(fd, PB2_INSERT, &val[0]);
                if(result > 0){
                    perror(RED "ERROR! Inserted while full\n" RESET);
                    return 0;
                }
                else {
                    printf(GRN "Success- insertion failed. " RESET);
                }
            }
            
        }
        else {
            printf("Extracting.. ");
            result = ioctl(fd, PB2_EXTRACT, &myresult);

            if(outputs[i] >= 0){
                if(myresult.result == outputs[i]){
                    printf(GRN "Match. " RESET);
                }
                else {
                    printf(RED "ERROR! Results Do Not Match. Expected %d, Found %d\n" RESET, (int)minsorted[i], (int)myresult.result);
                }
            }
            else {
                if(result > 0){
                    perror(RED "ERROR! Extracted while empty\n" RESET);
                    return 0;
                }
            }
        }
    }

    printf("\n");

    return 0;
}


int main(int argc, char *argv[]){
    if( argc != 2 ) {
        printf("Please provide your LKM proc file name as an argument.\nExample:\n./test_ass1 partb_1_<roll no>\n");
        return -1;
    }
    struct pb2_set_type_arguments mypb2_set_type_arguments;
    struct obj_info myobj_info;
    struct result myresult;


    char procfile[100] = "/proc/";
    strcat(procfile, argv[1]);

    int fd = open(procfile, O_RDWR);
    if(fd < 0){
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    basicMinHeapTest(fd);
    close(fd);


    fd = open(procfile, O_RDWR) ;
    if(fd < 0){
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    basicMaxHeapTest(fd);
    close(fd);

    fd = open(procfile, O_RDWR) ;
    if(fd < 0){
        perror("Could not open flie /proc/ass1");
        return 0;
    }
    largeTest(fd);
    close(fd);


    // Make two separate processes parallely use the lkm
    printf("\n============ TEST FOR MULTIPLE PROCESSES ==============\n");
    pid_t ppp;
    ppp = fork();

    fd = open(procfile, O_RDWR) ;
    if(fd < 0){
        perror("Could not open flie");
        return 0;
    }
    largeTest(fd);
    close(fd);

    if(ppp){
        wait(NULL);
    }

   return 0;
}