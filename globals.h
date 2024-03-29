#ifndef GLOBALS_H
#define GLOBALS_H

#define DEBUG

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
#include <boost/algorithm/string/replace.hpp>

using u1 = uint8_t;
using u2 = uint16_t;
using u4 = uint32_t;
using u8 = uint64_t;
using s1 = int8_t;
using s2 = int16_t;
using s4 = int32_t;
using s8 = int64_t;

namespace fs = std::filesystem;
using namespace std::string_literals;

struct Instruction
{
    u4 position;
    std::string opcode;
};

struct FunctionData
{
    std::string name;
    std::string descriptor;
    std::vector<Instruction> instructions;
    u2 flags;
    u1 returnFlags;
    std::vector<u1> parametersFlags;
};

struct FieldData
{
    std::string name;
    std::string type;
    bool isArray;
    u2 flags;
    std::optional<std::string> init = {};
};

enum
{
    aconst_null = 0x01,
    iconst_m1 = 0x02,
    iconst_0 = 0x03,
    iconst_1 = 0x04,
    iconst_2 = 0x05,
    iconst_3 = 0x06,
    iconst_4 = 0x07,
    iconst_5 = 0x08,
    fconst_0 = 0x0b,
    fconst_1 = 0x0c,
    fconst_2 = 0x0d,
    bipush = 0x10,
    sipush = 0x11,
    ldc = 0x12,
    ldc_w = 0x13,
    ldc2_w = 0x14,
    iload = 0x15,
    aload = 0x19,
    iload_0 = 0x1a,
    iload_1 = 0x1b,
    iload_2 = 0x1c,
    iload_3 = 0x1d,
    aload_0 = 0x2a,
    aload_1 = 0x2b,
    aload_2 = 0x2c,
    aload_3 = 0x2d,
    iaload = 0x2e,
    laload = 0x2f,
    faload = 0x30,
    daload = 0x31,
    aaload = 0x32,
    baload = 0x33,
    caload = 0x34,
    saload = 0x35,
    istore = 0x36,
    lstore = 0x37,
    fstore = 0x38,
    dstore = 0x39,
    astore = 0x3a,
    istore_0 = 0x3b,
    istore_1 = 0x3c,
    istore_2 = 0x3d,
    istore_3 = 0x3e,
    lstore_0 = 0x40,
    lstore_1 = 0x41,
    lstore_2 = 0x42,
    fstore_0 = 0x43,
    fstore_1 = 0x44,
    fstore_2 = 0x45,
    fstore_3 = 0x46,
    dstore_0 = 0x47,
    dstore_1 = 0x48,
    dstore_2 = 0x49,
    dstore_3 = 0x4a,
    astore_0 = 0x4b,
    astore_1 = 0x4c,
    astore_2 = 0x4d,
    astore_3 = 0x4e,
    iastore = 0x4f,
    lastore = 0x50,
    fastore = 0x51,
    dastore = 0x52,
    aastore = 0x53,
    bastore = 0x54,
    pop = 0x57,
    dup_ = 0x59,
    iadd = 0x60,
    ladd = 0x61,
    fadd_ = 0x62,
    dadd = 0x63,
    isub = 0x64,
    imul = 0x68,
    lmul = 0x69,
    fmul_ = 0x6a,
    dmul = 0x6b,
    idiv = 0x6c,
    irem = 0x70,
    ineg = 0x74,
    ishl = 0x78,
    iand = 0x7e,
    iinc = 0x84,
    i2f = 0x86,
    i2d = 0x87,
    l2f = 0x89,
    f2i = 0x8b,
    f2d = 0x8d,
    d2i = 0x8e,
    ifeq = 0x99,
    ifne = 0x9a,
    iflt = 0x9b,
    ifge = 0x9c,
    ifgt = 0x9d,
    ifle = 0x9e,
    if_icmpeq = 0x9f,
    if_icmpne = 0xa0,
    if_icmplt = 0xa1,
    if_icmpge = 0xa2,
    if_icmpgt = 0xa3,
    if_icmple = 0xa4,
    if_acmpeq = 0xa5,
    if_acmpne = 0xa6,
    goto_ = 0xa7,
    lookupswitch = 0xab,
    ireturn = 0xac,
    lreturn = 0xad,
    freturn = 0xae,
    dreturn = 0xaf,
    areturn = 0xb0,
    return_ = 0xb1,
    getstatic = 0xb2,
    putstatic = 0xb3,
    getfield = 0xb4,
    putfield = 0xb5,
    invokevirtual = 0xb6,
    invokespecial = 0xb7,
    invokestatic = 0xb8,
    invokedynamic = 0xba,
    new_ = 0xbb,
    newarray = 0xbc,
    anewarray = 0xbd,
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
    Badger2040,
    Gamebuino,
    Picosystem,
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

struct MethodType
{
    u2 descriptor_index;
};

struct Class
{
    u2 name_index;
};

struct String
{
    u2 string_index;
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

using Constant = std::variant<std::monostate
                            , Fieldref
                            , Methodref
                            , InterfaceMethodref
                            , Class
                            , NameAndType
                            , Utf8
                            , InvokeDynamic
                            , MethodHandle
                            , MethodType
                            , String
                            , float
                            , double
                            , s4
                            , s8>;
using ConstantPool = std::vector<Constant>;

u4 countArgs(std::string str);
std::string getReturnType(std::string descriptor, u1 flags);
std::string generateParameters(std::string descriptor, std::vector<u1> flags, bool isMethod);
Board getBoardTypeFromString(std::string board_name);
void copyUserFiles(std::filesystem::path currentPath);
std::string getTypeFromDescriptor(std::string descriptor, u8 flags);

#define STATIC_INIT "<clinit>"
#define CONSTRUCTOR "<init>"

enum class Format
{
    Rgb565,
    Indexed,
};

std::string javaToCpp(std::string name);
constexpr const char* RESOURCES_FILE = "resources";
constexpr const char* USER_FILE = "userdata";

constexpr u1 UNSIGNED_TYPE = 0x01;
constexpr u1 CONST_TYPE = 0x02;
constexpr u1 POINTER_TYPE = 0x04;

constexpr u2 ACC_PUBLIC = 0x0001;
constexpr u2 ACC_STATIC = 0x0008;
constexpr u2 ACC_FINAL = 0x0010;
constexpr u2 ACC_INTERFACE = 0x0200;

constexpr auto OBJ_INSTANCE = "local_0";

#endif // GLOBALS_H
