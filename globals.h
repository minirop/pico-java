#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <set>
#include <fstream>
#include <fmt/core.h>
#include <fmt/format.h>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <filesystem>
#include <set>
#include <utility>
#include <cassert>
#include <boost/algorithm/string/replace.hpp>

using u1 = uint8_t;
using u2 = uint16_t;
using u4 = uint32_t;
using s1 = int8_t;
using s2 = int16_t;
using s4 = int32_t;

struct Instruction
{
    u4 position;
    std::string opcode;
    std::optional<std::string> label;
};

struct FunctionData
{
    std::string name;
    std::string descriptor;
    std::vector<Instruction> instructions;
};

struct FieldData
{
    std::string name;
    std::string type;
};

extern std::vector<FunctionData> functions;
extern std::vector<FieldData> fields;

enum
{
    iconst_0 = 0x03,
    iconst_1 = 0x04,
    iconst_2 = 0x05,
    iconst_3 = 0x06,
    iconst_4 = 0x07,
    iconst_5 = 0x08,
    bipush = 0x10,
    sipush = 0x11,
    iload = 0x15,
    iload_0 = 0x1a,
    iload_1 = 0x1b,
    iload_2 = 0x1c,
    iload_3 = 0x1d,
    aload_0 = 0x2a,
    aload_1 = 0x2b,
    aload_2 = 0x2c,
    aload_3 = 0x2d,
    iaload = 0x2e,
    istore = 0x36,
    istore_0 = 0x3b,
    istore_1 = 0x3c,
    istore_2 = 0x3d,
    istore_3 = 0x3e,
    astore_0 = 0x4b,
    astore_1 = 0x4c,
    astore_2 = 0x4d,
    astore_3 = 0x4e,
    iastore = 0x4f,
    pop = 0x57,
    dup_ = 0x59,
    iadd = 0x60,
    irem = 0x70,
    ishl = 0x78,
    iinc = 0x84,
    ifne = 0x9a,
    if_icmpeq = 0x9f,
    if_icmpne = 0xa0,
    if_icmplt = 0xa1,
    if_icmpge = 0xa2,
    if_icmpgt = 0xa3,
    if_icmple = 0xa4,
    goto_ = 0xa7,
    iand = 0x7e,
    return_ = 0xb1,
    getstatic = 0xb2,
    putstatic = 0xb3,
    invokestatic = 0xb8,
    invokedynamic = 0xba,
    newarray = 0xbc,
    arraylength = 0xbe,
};

using Buffer = std::vector<u1>;
u1 read8(Buffer & buffer);
u2 read16(Buffer & buffer);
u4 read32(Buffer & buffer);

s1 reads8(Buffer & buffer);
s2 reads16(Buffer & buffer);
s4 reads32(Buffer & buffer);

#define r8()  read8(buffer)
#define r16() read16(buffer)
#define r32() read32(buffer)
#define s8()  reads8(buffer)
#define s16() reads16(buffer)
#define s32() reads32(buffer)

enum {
    CONSTANT_Utf8               = 1,
    CONSTANT_Integer            = 3,
    CONSTANT_Float              = 4,
    CONSTANT_Long               = 5,
    CONSTANT_Double             = 6,
    CONSTANT_Class              = 7,
    CONSTANT_String             = 8,
    CONSTANT_Fieldref           = 9,
    CONSTANT_Methodref          = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType        = 12,
    CONSTANT_MethodHandle       = 15,
    CONSTANT_MethodType         = 16,
    CONSTANT_InvokeDynamic      = 18,
};

enum {
    REF_getField         = 1,
    REF_getStatic        = 2,
    REF_putField         = 3,
    REF_putStatic        = 4,
    REF_invokeVirtual    = 5,
    REF_invokeStatic     = 6,
    REF_invokeSpecial    = 7,
    REF_newInvokeSpecial = 8,
    REF_invokeInterface  = 9,
};

enum class Board
{
    Pico,
    PicoW,
    Tiny2040,
    Tiny2040_2mb,
};

struct Fieldref
{
    u2 class_index;
    u2 name_and_type_index;
};

struct Methodref
{
    u2 class_index;
    u2 name_and_type_index;
};

struct InterfaceMethodref
{
    u2 class_index;
    u2 name_and_type_index;
};

struct InvokeDynamic
{
    u2 bootstrap_method_attr_index;
    u2 name_and_type_index;
};

struct MethodHandle
{
    u1 reference_kind;
    u2 reference_index;
};

struct Class
{
    u2 name_index;
};

struct NameAndType
{
    u2 name_index;
    u2 descriptor_index;
};

struct Utf8
{
    u2 length;
    Buffer bytes;
};

struct attribute_info
{
    u2 attribute_name_index;
    u4 attribute_length;
    Buffer info;
};

struct method_info
{
    u2 access_flags;
    u2 name_index;
    u2 descriptor_index;
    u2 attributes_count;
    std::vector<attribute_info> attributes;
};

using Constant = std::variant<Fieldref, Methodref, InterfaceMethodref, Class, NameAndType, Utf8, InvokeDynamic, MethodHandle>;
using ConstantPool = std::vector<Constant>;
extern ConstantPool constantPool;
extern std::vector<std::string> callbacksMethods;

std::string getStringFromUtf8(int index);
u4 countArgs(std::string str);
std::string getReturnType(std::string descriptor);

#define UNDEFINED_POSITION 999999

#endif // GLOBALS_H
