#include <unity.h>
#include "../../lib/Core/Battery.h" // We just want to test the RingBuffer nested struct

void test_ringbuffer_push_and_average(void) {
    RingBuffer rb;
    
    // Initial state
    TEST_ASSERT_EQUAL(0, rb.average());
    
    // Push some values
    rb.push(10);
    rb.push(20);
    rb.push(30);
    
    // Sum is 60, count is 3 => average is 20
    TEST_ASSERT_EQUAL(20, rb.average());
    
    // Push more to trigger wrap-around (RingBuffer capacity is 8)
    for (int i = 0; i < 10; i++) {
        rb.push(100);
    }
    
    // After 10 pushes, the entire buffer (8 slots) should be filled with 100
    TEST_ASSERT_EQUAL(100, rb.average());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_ringbuffer_push_and_average);
    UNITY_END();
    return 0;
}
