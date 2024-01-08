package main

import "regexp"

var OpcodeTypeMap map[string]OpCodes = map[string]OpCodes{
	"_Ctype_struct_WebLoginWorldServer_Struct":  OpCodes_Nested_WorldServer,
	"_Ctype_struct_CharacterSelectEntry_Struct": OpCodes_Nested_CharacterSelectEntry,
	"_Ctype_struct_CharSelectEquip_Struct":      OpCodes_Nested_CharSelectEquip,
	"_Ctype_struct_Tint_Struct":                 OpCodes_Nested_Tint,
	"_Ctype_struct_StringList":                  OpCodes_Nested_StringList,
}

var FixedArraySizePattern *regexp.Regexp = regexp.MustCompile("\\[(\\d+)\\]")
