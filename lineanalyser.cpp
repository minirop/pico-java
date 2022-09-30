#include "lineanalyser.h"
#include <bit>
#include <endian.h>
#include <optional>
#include <type_traits>
#include <cassert>

using namespace std::string_literals;

enum
{
    T_NONE    = 0,
    T_STRING  = 1,
    T_ARRAY   = 3,
    T_BOOLEAN = 4,
    T_CHAR    = 5,
    T_FLOAT   = 6,
    T_DOUBLE  = 7,
    T_BYTE    = 8,
    T_SHORT   = 9,
    T_INT     = 10,
    T_LONG    = 11,
};

std::string getType(int type)
{
    switch (type)
    {
    case T_BOOLEAN: return "bool";
    case T_CHAR:    return "char";
    case T_FLOAT:   return "float";
    case T_DOUBLE:  return "double";
    case T_BYTE:    return "char";
    case T_SHORT:   return "short";
    case T_INT:     return "int";
    case T_LONG:    return "long";
    }

    assert(false);
}

std::vector<Instruction> decodeBytecodeLine(Buffer & buffer, const std::string & name, u4 position);

std::vector<std::unordered_map<u4, u4>> localsTypes;
std::vector<Instruction> insts;
std::unordered_map<u4, u4> closingBraces;
std::set<u4> skippedGotos;
Buffer * fullBuffer = nullptr;

int findLocal(int index)
{
    for (auto it = localsTypes.rbegin(); it != localsTypes.rend(); ++it)
    {
        if (it->contains(index))
        {
            return it->operator [](index);
        }
    }

    return T_NONE;
}

std::vector<std::tuple<u2, u2>> * lines = nullptr;

u2 getLineFromOpcode(u2 opcode)
{
    auto & lineNumbers = *lines;

    for (size_t i = 0; i < lineNumbers.size() - 1; ++i)
    {
        auto pc = std::get<0>(lineNumbers[i]);
        auto nb = std::get<1>(lineNumbers[i]);
        if (opcode <= pc)
        {
            return nb;
        }
    }

    return std::get<1>(lineNumbers[lineNumbers.size() - 1]);
}

u2 getOpcodeFromLine(u2 opcode)
{
    for (auto & l : *lines)
    {
        if (std::get<1>(l) == opcode)
        {
            return std::get<0>(l);
        }
    }

    assert(false);
}

std::multiset<u4> closingBrackets, elseStmts;
std::vector<Instruction> lineAnalyser(Buffer & buffer, const std::string & name, std::vector<std::tuple<u2, u2>> lineNumbers)
{
    insts.clear();
    localsTypes.clear();
    localsTypes.push_back({});
    closingBraces.clear();

    fullBuffer = &buffer;
    lines = &lineNumbers;

    std::unordered_map<int, Buffer> buffers;
    std::set<int> order;
    for (size_t i = 0; i < lineNumbers.size(); ++i)
    {
        auto begin = std::get<0>(lineNumbers[i]);
        auto end = (i + 1 < lineNumbers.size() ? std::get<0>(lineNumbers[i + 1]) : buffer.size());
        auto sub = Buffer(&buffer[begin], &buffer[end]);

        auto line = std::get<1>(lineNumbers[i]);
        buffers[line].insert(
                    buffers[line].end(),
                    std::make_move_iterator(sub.begin()),
                    std::make_move_iterator(sub.end())
                    );
        order.insert(line);
    }

    for (auto key : order)
    {
        auto ret = decodeBytecodeLine(buffers[key], name, key);
        insts.insert(
                    insts.end(),
                    std::make_move_iterator(ret.begin()),
                    std::make_move_iterator(ret.end())
                    );
    }

    assert(closingBrackets.size() == 0);

    return insts;
}

struct Array
{
    size_t size;
    std::string type;
};

using Value = std::variant<int, long, float, double, std::string, Array>;
std::vector<Value> stack;
template<class> inline constexpr bool always_false_v = false;

std::string getAsString(const Value & value)
{
    return std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, long>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return fmt::format("{}", arg);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return arg;
        }
        else if constexpr (std::is_same_v<T, Array>)
        {
            return std::string{};
        }
        else
        {
            static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
    }, value);
}

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
        int index;
        std::optional<std::string> value;
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

std::string generateCodeFromOperation(Operation operation, u4 start_pc, bool & addOpeningParen);

std::vector<Instruction> decodeBytecodeLine(Buffer & buffer, const std::string & name, u4 position)
{
    assert(sizeof(int) != sizeof(long));

    auto buffer_size = buffer.size();
    auto start_pc = getOpcodeFromLine(position);

    std::vector<Operation> operations;
    std::vector<Instruction> lineInsts = { Instruction() };
    Instruction & inst = lineInsts.front();
    inst.position = position;

    while (buffer.size())
    {
        auto opcode = r8();

        switch (opcode)
        {
        case bipush:
        {
            auto value = r8();
            stack.push_back(value);
            break;
        }
        case anewarray:
        {
            auto val = stack.back();
            stack.pop_back();
            auto size = std::get<int>(val);
            auto type = r16();
            auto className = getStringFromUtf8(std::get<Class>(constantPool[type]).name_index);

            std::string cppType;
            if (className == "java/lang/String")
            {
                cppType = "std::string";
            }
            else
            {
                assert(false);
            }

            Array arr;
            arr.size = size;
            arr.type = cppType;
            stack.push_back(arr);
            break;
        }
        case astore_0:
        case astore_1:
        case astore_2:
        case astore_3:
        {
            int index = opcode - astore_0;
            auto v = stack.back();
            stack.pop_back();
            auto & locals = localsTypes.back();
            auto localType = findLocal(index);

            Operation op;
            op.type = OpType::Store;
            op.store.index = index;

            if (std::holds_alternative<Array>(v))
            {
                auto arr = std::get<Array>(v);

                op.store.size = arr.size;
                op.store.type = arr.type;

                if (localType != T_ARRAY)
                {
                    locals[index] = T_ARRAY;
                }
                else
                {
                    assert(false);
                }
            }
            else if (std::holds_alternative<std::string>(v))
            {
                auto str = std::get<std::string>(v);
                if (localType != T_STRING)
                {
                    op.store.type = "std::string";
                    locals[index] = T_STRING;
                }
            }

            operations.push_back(op);
            break;
        }
        case iconst_0:
        case iconst_1:
        case iconst_2:
        case iconst_3:
        case iconst_4:
        case iconst_5:
        {
            stack.push_back(opcode - iconst_0);
            break;
        }
        case fconst_0:
        case fconst_1:
        case fconst_2:
        {
            stack.push_back(static_cast<float>(opcode - fconst_0));
            break;
        }
        case istore:
        case istore_0:
        case istore_1:
        case istore_2:
        case istore_3:
        {
            auto v = stack.back();
            stack.pop_back();

            int index;
            if (opcode == istore)
            {
                index = r8();
            }
            else
            {
                index = opcode - istore_0;
            }

            Operation op;
            op.type = OpType::Store;
            op.store.index = index;

            auto & locals = localsTypes.back();
            auto localType = findLocal(index);
            if (localType != T_INT)
            {
                op.store.type = getType(T_INT);
                locals[index] = T_INT;
            }

            op.store.value = getAsString(v);

            operations.push_back(op);
            break;
        }
        case iload:
        {
            auto index = r8();
            stack.push_back(fmt::format("local_{}", index));
            break;
        }
        case iload_0:
        case iload_1:
        case iload_2:
        case iload_3:
        {
            int index = opcode - iload_0;
            stack.push_back(fmt::format("local_{}", index));
            break;
        }
        case aload_0:
        case aload_1:
        case aload_2:
        case aload_3:
        {
            int index = opcode - aload_0;
            stack.push_back(fmt::format("alocal_{}", index));
            break;
        }
        case arraylength:
        {
            auto val = stack.back();
            stack.pop_back();
            stack.push_back(fmt::format("{}.length()", std::get<std::string>(val)));
            break;
        }
        case if_icmpeq:
        case if_icmpne:
        case if_icmplt:
        case if_icmpge:
        case if_icmpgt:
        case if_icmple:
        case if_acmpeq:
        case if_acmpne:
        {
            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = start_pc + correction + offset;

            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            std::string binop;
            switch (opcode)
            {
            case if_icmpeq: binop = "!="; break;
            case if_icmpne: binop = "=="; break;
            case if_icmplt: binop = ">="; break;
            case if_icmpge: binop =  "<"; break;
            case if_icmpgt: binop = "<="; break;
            case if_icmple: binop =  ">"; break;
            case if_acmpeq: binop = "!="; break;
            case if_acmpne: binop = "=="; break;
            default: assert(false);
            }


            Operation op;
            op.type = OpType::Cond;
            op.cond.op = binop;
            op.cond.left = getAsString(left);
            op.cond.right = getAsString(right);
            op.cond.absolute = absolute;

            operations.push_back(op);
            break;
        }
        case iinc:
        {
            Operation op;
            op.type = OpType::Inc;
            op.inc.index = r8();
            op.inc.constant = r8();
            operations.push_back(op);
            break;
        }
        case goto_:
        {
            auto correction = buffer_size - buffer.size() - 1;
            auto offset = s16();
            auto absolute = start_pc + correction + offset;

            Operation op;
            op.type = OpType::Jump;
            op.jump.absolute = absolute;
            operations.push_back(op);
            break;
        }
        case invokedynamic:
        {
            auto id = r16();
            auto zero = r16();
            assert(zero == 0);

            auto invokeDyn = std::get<InvokeDynamic>(constantPool[id]);
            auto tpl = callbacksMethods[invokeDyn.bootstrap_method_attr_index];
            if (tpl.contains(0x02))
            {
                assert(false);
            }
            if (tpl.contains(0x01))
            {
                std::string newTpl = "\"";

                for (auto c : tpl)
                {
                    if (c == 0x01)
                    {
                        auto var = stack.back();
                        stack.pop_back();
                        newTpl += fmt::format("\" + {} + \"", getAsString(var));
                    }
                    else
                    {
                        newTpl += c;
                    }
                }
                newTpl += "\"";

                if (newTpl.starts_with("\"\" + "))
                {
                    newTpl = newTpl.substr(5);
                }

                if (newTpl.ends_with(" + \"\""))
                {
                    newTpl = newTpl.substr(0, newTpl.size() - 5);
                }
                tpl = newTpl;
            }
            stack.push_back(tpl);
            break;
        }
        case iastore:
        case aastore:
        {
            auto value = stack.back();
            stack.pop_back();
            auto index = stack.back();
            stack.pop_back();
            auto arr = stack.back();
            stack.pop_back();

            Operation op;
            op.type = OpType::IndexedStore;
            op.istore.index = getAsString(index);
            op.istore.array = getAsString(arr);
            op.istore.value = getAsString(value);

            operations.push_back(op);
            break;
        }
        case return_:
        {
            Operation op;
            op.type = OpType::Return;
            if (name == "main")
            {
                op.ret.value = "0";
            }
            operations.push_back(op);
            break;
        }
        case imul:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} * {})", getAsString(left), getAsString(right)));
            break;
        }
        case iadd:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} + {})", getAsString(left), getAsString(right)));
            break;
        }
        case invokestatic:
        {
            auto id = r16();
            auto method = std::get<Methodref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto methodName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
            auto fullName = fmt::format("{}::{}", className, methodName);
            boost::replace_all(fullName, "/"s, "::"s);

            std::string argsString;
            auto argsCount = countArgs(descriptor);
            if (argsCount && argsCount <= stack.size())
            {
                auto offset = stack.size() - argsCount;
                for (size_t idx = 0; idx < argsCount; ++idx)
                {
                    argsString += fmt::format(", {}", getAsString(stack[offset + idx]));
                }

                argsString = argsString.substr(2);

                auto tmpArgsCount = argsCount;
                while (tmpArgsCount > 0)
                {
                    --tmpArgsCount;
                    stack.pop_back();
                }
            }

            auto callString = fmt::format("{}({});", fullName, argsString);
            auto retType = getReturnType(descriptor);
            if (retType.size() && retType != "void")
            {
                stack.push_back(callString);
            }
            else
            {
                Operation op;
                op.type = OpType::Call;
                op.call.code = callString;
                operations.push_back(op);
            }
            break;
        }
        case getstatic:
        {
            auto id = r16();
            auto method = std::get<Fieldref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);
            auto variableName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);

            auto fullName = fmt::format("{}::{}", className, variableName);
            boost::replace_all(fullName, "/"s, "::"s);

            stack.push_back(fullName);
            break;
        }
        case sipush:
        {
            auto value = r16();
            stack.push_back(value);
            break;
        }
        case irem:
        {
            auto right = stack.back();
            stack.pop_back();
            auto left = stack.back();
            stack.pop_back();

            stack.push_back(fmt::format("({} % {})", getAsString(left), getAsString(right)));
            break;
        }
        case putstatic:
        {
            auto id = r16();
            auto method = std::get<Fieldref>(constantPool[id]);
            auto className = getStringFromUtf8(std::get<Class>(constantPool[method.class_index]).name_index);
            auto variableName = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).name_index);
            auto descriptor = getStringFromUtf8(std::get<NameAndType>(constantPool[method.name_and_type_index]).descriptor_index);

            auto fullName = fmt::format("{}::{}", className, variableName);
            boost::replace_all(fullName, "/"s, "::"s);

            auto val = stack.back();
            stack.pop_back();

            Operation op;
            op.type = OpType::Call;
            op.call.code = fmt::format("{} = {};", fullName, getAsString(val));
            operations.push_back(op);
            break;
        }
        default:
            assert(false);
        }
    }

    bool addOpeningParen = false;
    switch (operations.size())
    {
    case 1:
    {
        auto op = operations[0];
        inst.opcode = generateCodeFromOperation(op, start_pc, addOpeningParen);
        break;
    }
    case 2:
    {
        const auto & o1 = operations[0];
        const auto & o2 = operations[1];

        if (o2.type == OpType::Jump)
        {
            inst.opcode = generateCodeFromOperation(o1, start_pc, addOpeningParen);

            if (o2.jump.absolute > start_pc)
            {
                Instruction tmp;
                tmp.opcode = generateCodeFromOperation(o1, start_pc, addOpeningParen);
                lineInsts.push_back(tmp);
            }
            else
            {
                auto jumpLine = getLineFromOpcode(o2.jump.absolute);
                bool append = true;
                for (size_t idx = 0; idx < insts.size(); ++idx)
                {
                    if (jumpLine <= insts[idx].position)
                    {
                        Instruction tmp;
                        tmp.opcode = "while (true)";
                        insts.insert(insts.begin() + idx, tmp);
                        tmp.opcode = "{";
                        insts.insert(insts.begin() + idx + 1, tmp);
                        append = false;
                        break;
                    }
                }

                if (append)
                {
                    Instruction tmp;
                    tmp.opcode = "while (true)";
                    insts.push_back(tmp);
                    tmp.opcode = "{";
                    insts.push_back(tmp);
                }

                Instruction tmp;
                tmp.opcode = "}";
                lineInsts.push_back(tmp);
            }
        }
        else
        {
            auto op = operations[0];
            inst.opcode = generateCodeFromOperation(op, start_pc, addOpeningParen);

            Instruction tmp;
            tmp.opcode = generateCodeFromOperation(operations[1], start_pc, addOpeningParen);
            lineInsts.push_back(tmp);
        }
        break;
    }
    case 4:
    {
        const auto & o1 = operations[0];
        const auto & o2 = operations[1];
        const auto & o3 = operations[2];
        const auto & o4 = operations[3];

        // for loop
        if (o1.type == OpType::Store && o2.type == OpType::Cond && o3.type == OpType::Inc && o4.type == OpType::Jump)
        {
            std::string loop_var_type;
            if (o1.store.type.has_value())
            {
                loop_var_type = o1.store.type.value() + " ";
            }
            inst.opcode = fmt::format("for ({0}local_{1} = {2}; {3} {4} {5}; local_{6}",
                                      loop_var_type, o1.store.index, o1.store.value.value(),
                                      o2.cond.left, o2.cond.op, o2.cond.right,
                                      o3.inc.index);
            if (o3.inc.constant == 1)
            {
                inst.opcode += "++";
            }
            else
            {
                inst.opcode += fmt::format(" += {}", o3.inc.constant);
            }
            inst.opcode += ")";

            closingBrackets.insert(getLineFromOpcode(o2.cond.absolute));
            addOpeningParen = true;

            localsTypes.push_back({});
        }
        else
        {
            for (auto & op : operations)
            {
                fmt::print("{}\n", static_cast<int>(op.type));
            }
            assert(false);
        }
        break;
    }
    default:
    {
        assert(false);
    }
    }

    for (auto it = begin(elseStmts); it != end(elseStmts);)
    {
        if (*it <= position)
        {
            Instruction elseStmt;
            elseStmt.opcode = "{";
            lineInsts.insert(begin(lineInsts), elseStmt);
            elseStmt.opcode = "else";
            lineInsts.insert(begin(lineInsts), elseStmt);

            it = elseStmts.erase(it);

            localsTypes.pop_back();     // remove previous scope
            localsTypes.push_back({});  // add clean scope
            localsTypes.push_back({});  // add dummy scope that will get removed next loop
        }
        else
        {
            ++it;
        }
    }

    for (auto it = begin(closingBrackets); it != end(closingBrackets);)
    {
        if (*it <= position)
        {
            Instruction closingBracket;
            closingBracket.opcode = "}";
            lineInsts.insert(begin(lineInsts), closingBracket);
            it = closingBrackets.erase(it);

            localsTypes.pop_back();
        }
        else
        {
            ++it;
        }
    }

    if (addOpeningParen)
    {
        Instruction openingBracket;
        openingBracket.opcode = "{";
        lineInsts.push_back(openingBracket);
    }

    return lineInsts;
}

std::string generateCodeFromOperation(Operation operation, u4 start_pc, bool & addOpeningParen)
{
    std::string output;

    switch (operation.type)
    {
    case OpType::Store:
    {
        auto s = operation.store;
        std::string tmp;
        if (s.type.has_value())
        {
            tmp += s.type.value();
            tmp += " ";
        }
        tmp += fmt::format("local_{}", s.index);
        if (s.size.has_value())
        {
            tmp += fmt::format("[{}]", s.size.value());
        }
        else if (s.value.has_value())
        {
            tmp += fmt::format(" = {}", s.value.value());
        }

        output = tmp + ";";
        break;
    }
    case OpType::IndexedStore:
    {
        output = fmt::format("{}[{}] = {};", operation.istore.array, operation.istore.index, operation.istore.value);
        break;
    }
    case OpType::Return:
    {
        auto & val = operation.ret.value;
        if (val.has_value())
        {
            output = fmt::format("return {};", val.value());
        }
        else
        {
            output = fmt::format("return;");
        }
        break;
    }
    case OpType::Cond:
    {
        auto & c = operation.cond;
        auto absolute = c.absolute;
        auto isGoto = (*fullBuffer)[absolute - 3] == goto_;
        addOpeningParen = true;

        auto l = c.left;
        auto binop = c.op;
        auto r = c.right;

        if (isGoto)
        {
            s2 offset = (*fullBuffer)[absolute - 2] << 8 | (*fullBuffer)[absolute - 1];
            u4 target = absolute - 3 + offset;

            if (target == start_pc)
            {
                output = fmt::format("while ({} {} {})", l, binop, r);
                closingBrackets.insert(getLineFromOpcode(absolute));

                localsTypes.push_back({});
            }
            else if (target > start_pc)
            {
                output = fmt::format("if ({} {} {})", l, binop, r);
                closingBrackets.insert(getLineFromOpcode(absolute));

                localsTypes.push_back({});
            }
            else
            {
                assert(false);
            }
        }
        else
        {
            output = fmt::format("if ({} {} {})", l, binop, r);
            closingBrackets.insert(getLineFromOpcode(absolute));
        }
        break;
    }
    case OpType::Inc:
    {
        if (operation.inc.constant == 1)
        {
            output = fmt::format("local_{}++;", operation.inc.index);
        }
        else
        {
            output = fmt::format("local_{} += {};", operation.inc.index, operation.inc.constant);
        }
        break;
    }
    case OpType::Jump:
    {
        auto abs_line = getLineFromOpcode(operation.jump.absolute);
        auto curr_line = getLineFromOpcode(start_pc);
        if (operation.jump.absolute > start_pc)
        {
            if (abs_line < curr_line)
            {
                for (u4 pos = operation.jump.absolute; pos < fullBuffer->size(); ++pos)
                {
                    auto next_line = getLineFromOpcode(pos);
                    if (next_line > curr_line)
                    {
                        abs_line = next_line;
                        break;
                    }
                }
            }

            assert(abs_line > curr_line);

            elseStmts.insert(curr_line + 1);
            closingBrackets.insert(abs_line);
        }
        else
        {
            assert(false);
        }
        break;
    }
    case OpType::Call:
    {
        output = operation.call.code;
        break;
    }
    default:
        assert(false);
    }

    return output;
}
