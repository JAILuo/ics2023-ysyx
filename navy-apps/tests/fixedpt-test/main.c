#include "fixedptc.h"
#include <stdio.h>
#include <assert.h>

void test_fixedpt_muli(void) {
    fixedpt A = 0x12345678; // 模拟固定点数
    int B = 2;
    fixedpt result = fixedpt_muli(A, B);
    assert(result == 0x12345678 * B);
    printf("fixedpt_muli test passed\n");
}

void test_fixedpt_divi(void) {
    fixedpt A = 0x12345678;
    int B = 3;
    fixedpt result = fixedpt_divi(A, B);
    assert(result == A / B);
    printf("fixedpt_divi test passed\n");
}

void test_fixedpt_mul(void) {
    fixedpt A = 0x12345678;
    fixedpt B = 0x23456789;
    fixedpt result = fixedpt_mul(A, B);
    assert(result == ((A * B) >> 8));
    printf("fixedpt_mul test passed\n");
}

void test_fixedpt_div(void) {
    fixedpt A = 0x12345678;
    fixedpt B = 0x00000100; // 确保除法不会溢出
    fixedpt result = fixedpt_div(A, B);
    assert(result == ((A / B) << 8));
    printf("fixedpt_div test passed\n");
}

void test_fixedpt_abs(void) {
    fixedpt A = -0x12345678;
    fixedpt result = fixedpt_abs(A);
    assert(result == 0x12345678); // 检查结果是否正确
    printf("fixedpt_abs test passed\n");
}

void test_fixedpt_floor(void) {
    fixedpt A = 0x12345678;
    fixedpt result = fixedpt_floor(A);
    assert(result == (A & 0xffffff00)); // 检查结果是否正确
    printf("fixedpt_floor test passed\n");
}

void test_fixedpt_ceil(void) {
    fixedpt A = 0x12345678;
    fixedpt result = fixedpt_ceil(A);
    assert(result == ((A & 0xffffff00) + ((A & 0x000000ff) ? 0x00000100 : 0)));
    // 检查结果是否正确
    printf("fixedpt_ceil test passed\n");
}

int main() {
    printf("GPT test:\n");
    test_fixedpt_muli();
    test_fixedpt_divi();
    test_fixedpt_mul();
    test_fixedpt_div();
    test_fixedpt_abs();
    test_fixedpt_floor();
    test_fixedpt_ceil();
    printf("All tests passed successfully!\n");
    
    printf("test\n");
    fixedpt a = fixedpt_rconst(7.5); // 7 * 2^8 + 0.5 * 2^8 = 1920
    fixedpt b = fixedpt_rconst(-7.5);
    int c = 3;
    printf("%d\n",fixedpt_floor(a) >> 8);
    printf("%d\n",fixedpt_ceil(a) >> 8);
    printf("%d\n",fixedpt_floor(b) >> 8);
    printf("%d\n",fixedpt_ceil(b) >> 8);
    printf("b = %d abs(b) = %d\n", b, fixedpt_abs(b));

    // 1920 * 3 = 5760
    // 1920 * 1920 /256 = 14400 
    printf("muli1/divi1: %d %d\n", fixedpt_muli(a, c), fixedpt_divi(a, c));
    printf("muli2/divi2: %d %d\n", fixedpt_muli(b, c), fixedpt_divi(b, c));
    printf("mul/div: %d %d\n", fixedpt_mul(a, b), fixedpt_div(a, b));
    printf("%d\n", 0xfffffe00);

    return 0;
}

