test_common_src = files('common.c')
test_curl_dep = dependency('libcurl', required: false)
test_deps = [test_curl_dep]

test_get_index = executable(
    'test_get_index',
    test_common_src,
    files('test_get_index.c'),
    dependencies: test_deps,
)
test('index get', test_get_index, args: [exe.full_path()])

test_get_missing = executable(
    'test_get_missing',
    test_common_src,
    files('test_get_missing.c'),
    dependencies: test_deps,
)
test('missing 404', test_get_missing, args: [exe.full_path()])

test_post = executable(
    'test_post',
    test_common_src,
    files('test_post.c'),
    dependencies: test_deps,
)
test('post not implemented', test_post, args: [exe.full_path()])

test_traversal = executable(
    'test_traversal',
    test_common_src,
    files('test_traversal.c'),
    dependencies: test_deps,
)
test('traversal blocked', test_traversal, args: [exe.full_path()])

test_content_length = executable(
    'test_content_length',
    test_common_src,
    files('test_content_length.c'),
    dependencies: test_deps,
)
test('content length present', test_content_length, args: [exe.full_path()])