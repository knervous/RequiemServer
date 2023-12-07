package main

// #include "c_bridge.c"
import "C"

// func DoCallback(a unsafe.Pointer, cb C.SignalHandler) {
// 	var spawn C.struct_Spawn_Struct
// 	spawn.level = 55
// 	arr := [64]C.char{}
// 	mystr := "My new cool name"
// 	for i := 0; i < len(mystr) && i < 63; i++ { // leave element 256 at zero
// 		arr[i] = C.char(mystr[i])
// 	}
// 	spawn.name = arr
// 	spawn.anon = 1
// 	spawn.PlayerState = 3
// 	spawn.hairstyle = 22
// 	spawn.deity = 200
// 	spawn.bounding_radius = 1.567
// 	spawn.x = 66
// 	spawn.y = 77
// 	spawn.z = 88
// 	spew.Dump(spawn)
// 	C.bridge_int_int(a, cb)
// 	fmt.Printf("OK!")
// }
