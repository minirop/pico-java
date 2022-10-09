#ifndef CLASSFILE_H
#define CLASSFILE_H

#include "globals.h"

enum class OpType
{
    Store,
    Cond,
    Inc,
    Jump,
    IndexedStore,
    Return,
    Call,
};

struct Operation
{
    OpType type;

    struct
    {
        std::optional<std::string> type;
        std::optional<int> size;
        u4 position;
        std::string arr_type;
        int index;
        std::optional<std::string> value;
        std::vector<std::string> populate;
    } store;

    struct
    {
        std::string left;
        std::string right;
        u4 absolute;
        std::string op;
    } cond;

    struct
    {
        int index;
        int constant;
    } inc;

    struct
    {
        u4 absolute;
    } jump;

    struct
    {
        std::string array;
        std::string index;
        std::string value;
    } istore;

    struct
    {
        std::optional<std::string> value;
    } ret;

    struct
    {
        std::string code;
    } call;
};

struct Array
{
    size_t size;
    std::string type;
    size_t position;
    std::vector<std::string> populate;
};

struct Object
{
    std::string type;
    std::string ctor;
};

using Value = std::variant<int32_t, int64_t, float, double, std::string, Array, Object>;

class ClassFile
{
public:
    ClassFile(std::string filename, std::string projectName, bool partial = false);

    bool hasBoard() const;
    std::string boardName() const;
    void generate(const std::vector<ClassFile> & files, Board board);

    std::vector<Instruction> lineAnalyser(Buffer & buffer, const std::string & name, std::vector<std::tuple<u2, u2>> lineNumbers);
    std::vector<Instruction> decodeBytecodeLine(Buffer & buffer, const std::string & name, u4 position);
    std::string generateCodeFromOperation(Operation operation, u4 start_pc, bool & addOpeningParen);
    u2 getLineFromOpcode(u2 opcode);
    u2 getOpcodeFromLine(u2 line);
    int findLocal(int index);
    std::string getStringFromUtf8(int index);

    std::vector<FunctionData> functions;
    std::vector<FieldData> fields;
    ConstantPool constantPool;
    std::vector<std::string> callbacksMethods;
    std::vector<std::unordered_map<u4, u4>> localsTypes;
    std::vector<Instruction> insts;
    std::unordered_map<u4, u4> closingBraces;
    std::set<u4> skippedGotos;
    std::multiset<u4> closingBrackets, elseStmts;
    std::vector<Value> stack;
    std::string board_name;
    std::string fileName;
    Buffer * fullBuffer = nullptr;
    std::vector<std::tuple<u2, u2>> * lines = nullptr;
    std::string project_name;
};

#endif // CLASSFILE_H
