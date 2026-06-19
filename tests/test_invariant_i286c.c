#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

/* We need to test that i286c_initialize (or equivalent memory allocation)
   rejects sizes that would cause integer overflow in (size + 16). */

/* Include the header that declares the memory init function */
#include "i286c/i286c.h"

START_TEST(test_memory_allocation_overflow_guard)
{
    /* Invariant: Memory allocation size must never wrap around due to
       integer overflow. Sizes near UINT_MAX must be rejected or clamped. */

    UINT sizes[] = {
        (UINT)(UINT_MAX),          /* Exact exploit: size+16 wraps to 15 */
        (UINT)(UINT_MAX - 15),     /* Boundary: size+16 wraps to 0 */
        (UINT)(UINT_MAX - 7),      /* Near-overflow: size+16 wraps to 8 */
        (UINT)(1024 * 1024),       /* Valid: 1MB, normal emulator memory */
        (UINT)(16 * 1024 * 1024),  /* Valid: 16MB, large but reasonable */
    };
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < num_sizes; i++) {
        UINT size = sizes[i];
        /* Check for overflow: if size + 16 would wrap, the call must fail
           gracefully (return error / not crash). For valid sizes it should
           succeed. */
        if (size > (UINT)(UINT_MAX - 16)) {
            /* Overflow case: calling i286c_initialize should either return
               an error code or handle gracefully without allocating an
               undersized buffer. We verify no crash occurs. */
            int ret = i286c_initialize(size);
            /* The function must reject this - nonzero return or safe handling */
            ck_assert_msg(ret != 0 || size <= (UINT_MAX - 16),
                "i286c_initialize accepted overflow-prone size 0x%x", size);
            i286c_deinitialize();
        } else {
            /* Valid size: should succeed */
            int ret = i286c_initialize(size);
            ck_assert_msg(ret == 0,
                "i286c_initialize failed for valid size 0x%x", size);
            i286c_deinitialize();
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_memory_allocation_overflow_guard);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}