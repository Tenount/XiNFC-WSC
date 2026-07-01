#include <setjmp.h>
// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include <cmocka.h>
// clang-format on

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

static void test_flock_exclusive(void** state) {
    (void)state;

    char tmpl[] = "/tmp/xinfc-locktest.XXXXXX";
    int fd = mkstemp(tmpl);
    assert_true(fd >= 0);

    assert_int_equal(flock(fd, LOCK_EX | LOCK_NB), 0);

    int fd2 = open(tmpl, O_RDWR);
    assert_true(fd2 >= 0);

    assert_int_equal(flock(fd2, LOCK_EX | LOCK_NB), -1);
    assert_int_equal(errno, EWOULDBLOCK);
    close(fd2);

    assert_int_equal(flock(fd, LOCK_UN), 0);
    assert_int_equal(flock(fd, LOCK_EX | LOCK_NB), 0);

    close(fd);
    unlink(tmpl);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_flock_exclusive),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
