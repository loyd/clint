/*!
 * @brief Tests for the rules.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <json.h>

#include "clint.h"
#include "helper.h"


static void setup(const char *config)
{
    if (g_config)
        json_value_free(g_config);

    g_config = json_parse(config, strlen(config));
    assert(g_config);
    assert(configure_rules());
}


static void check(const char *input, bool full, int expected)
{
    static char buffer[512];
    int actual;

    if (full)
        g_data = (char *)input;
    else
    {
        snprintf(buffer, sizeof(buffer), "void test() {\n%s\n}", input);
        g_data = buffer;
    }

    init_parser();
    parse();
    check_rules();

    actual = g_errors ? vec_len(g_errors) : 0;
    if (expected != actual)
    {
        fprintf(stderr, "Expected (%d) != actual (%d).\n", expected, actual);

        for (int i = 0; i < actual; ++i)
            fprintf(stderr, "  - %s (%u:%u)\n", g_errors[i].message,
                g_errors[i].line + 1, g_errors[i].column + 1);

        assert(0);
    }

    g_data = NULL;
    reset_state();
}


static void test_block(void)
{
    group("block rules");

    test("disallow-empty");
    setup("{ \"block\": { \"disallow-empty\": true }}");
    check("if (a) {}", false, 1);
    check("if (a) {/*comment*/}", false, 1);
    check("if (a) {t;} else {}", false, 1);
    check("while (a) {}", false, 1);
    check("for (;;) {}", false, 1);
    check("{}", false, 1);
    check("{ a; }", false, 0);

    test("disallow-short");
    setup("{ \"block\": { \"disallow-short\": true }}");
    check("if (a) { b; }", false, 1);
    check("while (a) { b; }", false, 1);
    check("for (;;) { b; }", false, 1);
    check("switch (a) { case A: break; }", false, 0);
    check("if (a) { b; c; }", false, 0);

    test("disallow-oneline");
    setup("{ \"block\": { \"disallow-oneline\": true }}");
    check("if (a) b;", false, 1);
    check("while (a) b;", false, 1);
    check("for (;;) b;", false, 1);
    check("do b; while (a);", false, 1);
    check("while (a) { a; \n b;}", false, 0);
    check("while (a) \n{ a; b;}", false, 0);

    test("require-decls-on-top");
    setup("{ \"block\": { \"require-decls-on-top\": true }}");
    check("void foo() { int a; b(); }", true, 0);
    check("void foo() { b(); int a; }", true, 1);

    test("allow-before-decls");
    setup("{ \"block\": { \"require-decls-on-top\": true,"
                         "\"allow-before-decls\": [\"assert\"] }}");
    check("void foo() { assert(); int a; b; }", true, 0);
    check("void foo() { assert(); b; int a; }", true, 1);
}


static void test_indentation(void)
{
    //#TODO: more test cases.
    // group("indentation rules");
}


static void test_lines(void)
{
    group("lines rules");

    test("maximum-length");
    setup("{ \"lines\": { \"maximum-length\": 20 }}");
    check("void t() { int a; }", true, 0);
    check("void t() { int ab; }", true, 0);
    check("void t() { int abc; }", true, 1);

    test("disallow-trailing-space");
    setup("{ \"lines\": { \"disallow-trailing-space\": true }}");
    check("int i;", true, 0);
    check("int i;  ", true, 1);
    check("int i;\t", true, 1);
    check("int i;\n\t\n", true, 1);
    check("char *str = \"multi \\\n line\";", true, 0);

    test("require-line-break");
    setup("{ \"lines\": { \"require-line-break\": \"\\n\" }}");
    check("void foo() {\n}", true, 0);
    check("void foo() {\r}", true, 1);
    check("void foo() {\r\n\r}", true, 1);
    setup("{ \"lines\": { \"require-line-break\": \"\\r\" }}");
    check("void foo() {\n}", true, 1);
    check("void foo() {\r}", true, 0);
    check("void foo() {\rd\nd\r}", true, 1);
    // check("void foo() {\r\n\r}", true, 1);
    setup("{ \"lines\": { \"require-line-break\": \"\\r\\n\" }}");
    check("void foo() {\r\n}", true, 0);
    check("void foo() {\r\na\r}", true, 1);
    // check("void foo() {\r\n\r}", true, 1);

    test("require-newline-at-eof");
    setup("{ \"lines\": { \"require-newline-at-eof\": true }}");
    check("void foo() {}", true, 1);
    check("void foo() {}\n", true, 0);
    check("void foo() {}\r\n", true, 0);
}


static void test_naming(void)
{
    group("naming rules");

    test("global-var-prefix");
    setup("{ \"naming\": { \"global-var-prefix\": \"g_\" }}");
    check("static int foo;", true, 0);
    check("extern int foo;", true, 0);
    check("int foo;", true, 1);
    check("t_t foo;", true, 1);
    setup("{ \"naming\": { \"global-var-prefix\": \"\" }}");
    check("int a;", true, 0);

    test("global-fn-prefix");
    setup("{ \"naming\": { \"global-fn-prefix\": \"g_\" }}");
    check("static int foo() {}", true, 0);
    check("extern int foo();", true, 0);
    check("int foo() {}", true, 1);
    check("t_t foo() {}", true, 1);
    setup("{ \"naming\": { \"global-fn-prefix\": \"\" }}");
    check("int a() {}", true, 0);

    test("typedef-suffix");
    setup("{ \"naming\": { \"typedef-suffix\": \"_t\" }}");
    check("typedef int foo;", true, 1);
    check("typedef int foo_t;", true, 0);
    setup("{ \"naming\": { \"typedef-suffix\": \"\" }}");
    check("typedef int foo;", true, 0);

    test("struct-suffix");
    setup("{ \"naming\": { \"struct-suffix\": \"_s\" }}");
    check("struct foo {};", true, 1);
    check("struct foo_s {};", true, 0);
    check("struct foo a;", true, 0);
    setup("{ \"naming\": { \"struct-suffix\": \"\" }}");
    check("struct foo {};", true, 0);
    check("struct foo a;", true, 0);

    test("union-suffix");
    setup("{ \"naming\": { \"union-suffix\": \"_u\" }}");
    check("union foo {};", true, 1);
    check("union foo_u {};", true, 0);
    check("union foo a;", true, 0);
    setup("{ \"naming\": { \"union-suffix\": \"\" }}");
    check("union foo {};", true, 0);
    check("union foo a;", true, 0);

    test("enum-suffix");
    setup("{ \"naming\": { \"enum-suffix\": \"_e\" }}");
    check("enum foo {};", true, 1);
    check("enum foo_e {};", true, 0);
    check("enum foo a;", true, 0);
    setup("{ \"naming\": { \"enum-suffix\": \"\" }}");
    check("enum foo {};", true, 0);
    check("enum foo a;", true, 0);

    test("require-style");
    setup("{ \"naming\": { \"require-style\": \"under_score\" }}");
    check("int foo() {}", true, 0);
    check("int foo_bar() {}", true, 0);
    check("int fooBar() {}", true, 1);
    setup("{ \"naming\": { \"require-style\": \"none\" }}");
    check("int foo_bar() {}", true, 0);
    check("int fooBar() {}", true, 0);

    test("minimum-length");
    setup("{ \"naming\": { \"minimum-length\": 2 }}");
    check("int a;", false, 1);
    check("int a;", true, 1);
    check("struct a {};", true, 1);
    check("struct a {};", false, 1);
    check("int ab;", false, 0);
    check("int ab;", true, 0);
    check("int abc;", true, 0);
    check("for (int i;;) {}", false, 1);
    check("static int a;", false, 1);
    check("static int a;", true, 1);
    check("extern int a;", false, 0);
    check("extern int a;", true, 0);
    check("struct s abc;", false, 0);
    check("struct s abc;", true, 0);
    check("t abc;", false, 0);
    check("t abc;", true, 0);

    test("allow-short-on-top");
    setup("{ \"naming\": {"
        "\"minimum-length\": 2, \"allow-short-on-top\": true }}");
    check("int a;", true, 0);
    check("struct a {};", true, 0);

    test("allow-short-in-loop");
    setup("{ \"naming\": {"
        "\"minimum-length\": 2, \"allow-short-in-loop\": true }}");
    check("for (int i;;) {}", false, 0);

    test("allow-short-in-block");
    setup("{ \"naming\": {"
        "\"minimum-length\": 2, \"allow-short-in-block\": true }}");
    check("int a;", false, 0);
    check("struct a {};", false, 0);

    test("disallow-leading-underscore");
    setup("{ \"naming\": { \"disallow-leading-underscore\": true }}");
    check("int _a;", true, 1);
    check("static int _a;", true, 1);
    check("struct _a {};", true, 1);
    check("union _a {};", true, 1);
    check("enum _a {};", true, 1);
    check("extern int _a;", true, 0);
    check("_t a;", true, 0);
    check("struct _a t;", true, 0);
    check("union _a t;", true, 0);
    check("enum _a t;", true, 0);
    check("_t foo() {}", true, 0);
}


static void test_runtime(void)
{
    group("runtime rules");

    test("require-threadsafe-fn");
    setup("{ \"runtime\": { \"require-threadsafe-fn\": true }}");
    check("rand();", false, 1);

    test("require-safe-fn");
    setup("{ \"runtime\": { \"require-safe-fn\": true }}");
    check("gets();", false, 1);

    test("require-sized-int");
    setup("{ \"runtime\": { \"require-sized-int\": true }}");
    check("long a;", false, 1);
    check("int a;", false, 0);
    check("unsigned a;", false, 0);

    test("require-sizeof-as-fn");
    setup("{ \"runtime\": { \"require-sizeof-as-fn\": true }}");
    check("sizeof a;", false, 1);
    check("sizeof(a);", false, 0);
}


static void test_whitespace(void)
{
    group("whitespace rules");

    test("after-control");
    setup("{ \"whitespace\": { \"after-control\": false }}");
    check("while (a) {}", false, 1);
    check("if (a) {}", false, 1);
    check("for (;;) {}", false, 1);
    check("switch (a) {}", false, 1);
    setup("{ \"whitespace\": { \"after-control\": true }}");
    check("while(a) {}", false, 1);
    check("if(a) {}", false, 1);
    check("for(;;) {}", false, 1);
    check("switch(a) {}", false, 1);

    test("before-control");
    setup("{ \"whitespace\": { \"before-control\": false }}");
    check("if (a) {} else c;", false, 1);
    check("do {} while (d);", false, 1);
    setup("{ \"whitespace\": { \"before-control\": true }}");
    check("if (a) {}else c;", false, 1);
    check("do {}while (d);", false, 1);

    test("before-comma");
    setup("{ \"whitespace\": { \"before-comma\": false }}");
    check("(1 , 2);", false, 1);
    check("(1 , 2 ,3);", false, 2);
    check("(1 , 2 ,3);", false, 2);
    setup("{ \"whitespace\": { \"before-comma\": true }}");
    check("(1, 2);", false, 1);
    check("(1, 2,3);", false, 2);
    check("(1, 2,3);", false, 2);

    test("after-comma");
    setup("{ \"whitespace\": { \"after-comma\": false }}");
    check("(1, 2);", false, 1);
    check("(1, 2 , 3);", false, 2);
    check("(1, 2 , 3);", false, 2);
    setup("{ \"whitespace\": { \"after-comma\": true }}");
    check("(1,2);", false, 1);
    check("(1,2,3);", false, 2);
    check("(1,2,3);", false, 2);

    test("after-left-paren");
    setup("{ \"whitespace\": { \"after-left-paren\": false }}");
    check("void foo( a );", true, 1);
    setup("{ \"whitespace\": { \"after-left-paren\": true }}");
    check("void foo(a);", true, 1);

    test("before-right-paren");
    setup("{ \"whitespace\": { \"before-right-paren\": false }}");
    check("void foo( a );", true, 1);
    setup("{ \"whitespace\": { \"before-right-paren\": true }}");
    check("void foo(a);", true, 1);

    test("after-left-square");
    setup("{ \"whitespace\": { \"after-left-square\": false }}");
    check("a[ 2 ];", false, 1);
    setup("{ \"whitespace\": { \"after-left-square\": true }}");
    check("a[2];", false, 1);

    test("before-right-square");
    setup("{ \"whitespace\": { \"before-right-square\": false }}");
    check("a[ 2 ];", false, 1);
    setup("{ \"whitespace\": { \"before-right-square\": true }}");
    check("a[2];", false, 1);

    test("before-semicolon");
    setup("{ \"whitespace\": { \"before-semicolon\": false }}");
    check("a ;", false, 1);
    setup("{ \"whitespace\": { \"before-semicolon\": true }}");
    check("a;", false, 1);

    test("after-semicolon");
    setup("{ \"whitespace\": { \"after-semicolon\": false }}");
    check("; d;", false, 1);
    setup("{ \"whitespace\": { \"after-semicolon\": true }}");
    check(";d;", false, 1);

    test("require-block-on-newline");
    setup("{ \"whitespace\": { \"require-block-on-newline\": true }}");
    check("void foo() \n{\n}", true, 0);
    check("void foo() {a;\n}", true, 1);
    check("void foo() {}", true, 2);

    test("newline-before-members");
    setup("{ \"whitespace\": { \"newline-before-members\": false }}");
    check("struct \n{};", true, 1);
    check("struct {};", true, 0);
    setup("{ \"whitespace\": { \"newline-before-members\": true }}");
    check("struct \n{};", true, 0);
    check("struct {};", true, 1);

    test("newline-before-block");
    setup("{ \"whitespace\": { \"newline-before-block\": false }}");
    check("if (a) \n{}", false, 1);
    check("if (a) {}", false, 0);
    setup("{ \"whitespace\": { \"newline-before-block\": true }}");
    check("if (a) \n{}", false, 0);
    check("if (a) {}", false, 1);

    //#TODO: repair newline-before-control.
    // test("newline-before-control");
    // setup("{ \"whitespace\": { \"newline-before-control\": false }}");
    // check("do {\n} while (a);", false, 0);
    // // check("if (a) {}", false, 0);
    // setup("{ \"whitespace\": { \"newline-before-control\": true }}");
    // check("do {} while (a);", false, 1);

    test("newline-before-fn-body");
    setup("{ \"whitespace\": { \"newline-before-fn-body\": false }}");
    check("void foo(a) \n{}", true, 1);
    check("void foo(a) {}", true, 0);
    setup("{ \"whitespace\": { \"newline-before-fn-body\": true }}");
    check("void foo(a) \n{}", true, 0);
    check("void foo(a) {}", true, 1);

    test("between-unary-and-operand");
    setup("{ \"whitespace\": { \"between-unary-and-operand\": false }}");
    check("+b;", false, 0);
    check("+ b;", false, 1);
    check("sizeof b;", false, 0);
    check("sizeof  b;", false, 1);
    setup("{ \"whitespace\": { \"between-unary-and-operand\": true }}");
    check("+b;", false, 1);
    check("+ b;", false, 0);
    check("sizeof  b;", false, 1);

    test("around-binary");
    setup("{ \"whitespace\": { \"around-binary\": false }}");
    check("a+b;", false, 0);
    check("a -b;", false, 1);
    check("a/ b;", false, 1);
    check("c = a * b;", false, 2);
    setup("{ \"whitespace\": { \"around-binary\": true }}");
    check("a-b;", false, 2);
    check("a /b;", false, 1);
    check("a% b;", false, 1);
    check("c = a * b;", false, 0);

    test("around-bitwise");
    setup("{ \"whitespace\": { \"around-bitwise\": false }}");
    check("a = b|c;", false, 0);
    check("a = b &c;", false, 1);
    check("a = b| c;", false, 1);
    check("a = b & c;", false, 2);
    setup("{ \"whitespace\": { \"around-bitwise\": true }}");
    check("a = b|c;", false, 2);
    check("a = b &c;", false, 1);
    check("a = b| c;", false, 1);
    check("a = b & c;", false, 0);

    test("around-assignment");
    setup("{ \"whitespace\": { \"around-assignment\": false }}");
    check("a=b;", false, 0);
    check("a +=b;", false, 1);
    check("a/= b;", false, 1);
    check("a *= b;", false, 2);
    setup("{ \"whitespace\": { \"around-assignment\": true }}");
    check("a/=b;", false, 2);
    check("a %=b;", false, 1);
    check("a= b;", false, 1);
    check("a += b;", false, 0);

    test("around-accessor");
    setup("{ \"whitespace\": { \"around-accessor\": false }}");
    check("a.b;", false, 0);
    check("a ->b;", false, 1);
    check("a. b;", false, 1);
    check("a -> b;", false, 2);
    setup("{ \"whitespace\": { \"around-accessor\": true }}");
    check("a->b;", false, 2);
    check("a .b;", false, 1);
    check("a-> b;", false, 1);
    check("a . b;", false, 0);

    test("in-conditional");
    setup("{ \"whitespace\": { \"in-conditional\": false }}");
    check("a ? b : c;", false, 4);
    check("a ?b : c;", false, 3);
    check("a ?b :c;", false, 2);
    check("a ?b:c;", false, 1);
    check("a?b:c;", false, 0);
    setup("{ \"whitespace\": { \"in-conditional\": true }}");
    check("a ? b : c;", false, 0);
    check("a ?b : c;", false, 1);
    check("a ?b :c;", false, 2);
    check("a ?b:c;", false, 3);
    check("a?b:c;", false, 4);

    test("after-cast");
    setup("{ \"whitespace\": { \"after-cast\": false }}");
    check("(int)a;", false, 0);
    check("(int) a;", false, 1);
    setup("{ \"whitespace\": { \"after-cast\": true }}");
    check("(int)a;", false, 1);
    check("(int) a;", false, 0);

    test("in-call");
    setup("{ \"whitespace\": { \"in-call\": false }}");
    check("foo();", false, 0);
    check("foo ();", false, 1);
    setup("{ \"whitespace\": { \"in-call\": true }}");
    check("foo();", false, 1);
    check("foo ();", false, 0);

    test("after-name-in-fn-def");
    setup("{ \"whitespace\": { \"after-name-in-fn-def\": false }}");
    check("void foo() {}", true, 0);
    check("void foo () {}", true, 1);
    setup("{ \"whitespace\": { \"after-name-in-fn-def\": true }}");
    check("void foo() {}", true, 1);
    check("void foo () {}", true, 0);

    test("before-declarator-name");
    setup("{ \"whitespace\": { \"before-declarator-name\": true }}");
    check("struct {} a;", true, 0);
    check("struct {}a;", true, 1);
    check("int *a;", true, 0);

    test("before-members");
    setup("{ \"whitespace\": { \"before-members\": false }}");
    check("struct test {};", true, 1);
    setup("{ \"whitespace\": { \"before-members\": true }}");
    check("struct test{};", true, 1);

    test("pointer-place");
    setup("{ \"whitespace\": { \"pointer-place\": \"free\" }}");
    check("int *a;", true, 0);
    check("int * a;", true, 0);
    check("int* a;", true, 0);
    setup("{ \"whitespace\": { \"pointer-place\": \"declarator\" }}");
    check("int *a;", true, 0);
    check("int * a;", true, 1);
    check("int* a;", true, 2);
    setup("{ \"whitespace\": { \"pointer-place\": \"type\" }}");
    check("int *a;", true, 2);
    check("int * a;", true, 1);
    check("int* a;", true, 0);
    setup("{ \"whitespace\": { \"pointer-place\": \"middle\" }}");
    check("int *a;", true, 1);
    check("int * a;", true, 0);
    check("int* a;", true, 1);

    //#TODO: add more cases.
    test("allow-alignment");
    setup("{ \"whitespace\": { \"allow-alignment\": true, "
                              "\"around-assignment\": true }}");
    check("a  = 10;\nab = 20;", false, 0);
}


void test_rules(void)
{
    test_block();
    test_indentation();
    test_lines();
    test_naming();
    test_runtime();
    test_whitespace();
}
