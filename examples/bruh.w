/*
	
*/

// variables
foo := S32;   // default-initialized as `0`
foo: S32;     // uninitialized
foo: S32 = 0; // initialized as `0`
foo := 0;     // inferred typed

// constants
FOO :: S32;   // default-initialized as `0`
FOO: S32 = 0; // initialized as `0`

// procedures
do_foo :: proc;                                       // empty
do_foo :: proc(foo: S32);                             // with parameters
do_foo :: proc(foo: S32) -> S32;                      // with return value
do_foo :: proc(foo: S32) -> S32 {return foo};        // returning
do_foo :: proc(foo: S32) -> (bar: S32);               // with named return value
do_foo :: proc(foo: S32) -> (bar: S32) {bar = foo};  // returning by setting

// structures
Foo :: struct;                            // empty
Foo :: struct {Size; Size};               // anonymous fields
Foo :: struct {Size = 0; Size = 0};       // default initialized
Point :: struct {x: Size = 0; y: Size = 0}; // named

// String :: struct {value: []U8; length: Size};

// enumerations
Foo :: enum {FALSE = 0; TRUE = 1};                       // default type: Size (elements _must_ be set)
Foo :: enum -> S32 {FALSE = 0; TRUE = 1};                // with specified type
Foo :: enum -> String {HELLO = "Hello"; WORLD = "World"} // any type works as long as the data are different.
Foo :: enum -> Point {ORIGIN = {0, 0}, TOWN = {7, 21}, FREAKY_TOWN = {69, 420}};

// unions
Foo :: union {a: U32; b: F32; c: String};

Vector3 :: union {
	struct {x, y, z: F32};
	struct {r, g, b: F32};
};

// named scope
Foo :: {
	// What a cool scope!
};

// aliases
Dude === Vector3;
y === x := 69;

// conditions
x := true ? "Foo" : "Bar";

true ? {

}, false ? {

} : {
};

Day :: enum {SUNDAY = 0, MONDAY = 1, TUESDAY = 2, WEDNSDAY = 3};
day: Day = 'SUNDAY;

// switches
message := day == {
	.SUNDAY                                 ? "bruh.... we got fucking school tomorrow",
	.MONDAY, .TUESDAY, .WEDNSDAY, .THURSDAY ? "fuck school",
	.FRIDAY                                 ? "let's go! i can stay up today cuz there's no school tomorrow!",
	.SATURDAY                               ? "yay! no school!",
} : "<unknown day>";

array: []S32 = (0, 1, 2);

tuple: (S32, S32), tuple2: (U8, U8, U8) = (7, 21), (1, 2, 3);

result: S32;
result, successful: Bool = do_foo;

(r, g, b): U8, color: (U8, U8, U8) = 0, 0, 0, (0, 0, 0);

// labels
{
	i := 0;
[a]
	i == 10 ? jump_to z;
	print "%i", i;
	i += 1;
	jump_to a;
[z]
}


{
[a]
	{
	[a]
		jump_to a;   // jumps to the `a` from within the same scope
		jump_to ..a; // jumps to the `a` from within the prior scope
	}
}

// loops
// none! lmfao!
