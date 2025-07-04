
#pragma once

#include "lexer.hpp"

#include <variant>
#include <sstream>
#include <optional>

enum class IRType
{
    LOAD_NUMBER,
    LOAD_STACK,
    DEREF,
    ASSIGN,
    ADD,
    SUB,
    MUL,
    DIV,
};

using IRValue  = std::variant<IRType, std::string>;
using IRValues = std::vector<IRValue>;
using IRLocalInfo = std::unordered_map<std::string, size_t>;


enum {
    IR_TYPE,
    IR_STRING,
    IR_INT
};

struct IRInfo
{
    std::unordered_map<std::string, IRValues> labels;
    std::vector<std::string> refrencing;
    std::vector<std::string> strings;
    size_t stringPtr;
};

class IRGenerator
{
public:
    IRInfo Generate(const std::vector<Token>& tokens);
    bool PrintErrors();
    void SetSourceName(const std::string& name);

private:
    // IR Info
    IRInfo m_irInfo;
    std::vector<Token> m_tokens;
    size_t m_tokenIndex;
    std::stringstream m_errors;
    bool m_gotError;
    std::string m_sourceName;
    std::string m_currentLabel;
    std::unordered_map<std::string, int> m_params;
    std::vector<IRLocalInfo> m_locals;
    std::vector<size_t> m_localsSizeStack;
    size_t m_localsSize;

    // Source Functions
    Token& At();
    Token& Eat();
    TokenType Type();
    void Advance();
    Token& Expect(TokenType type, const std::string& messsage);
    Token& Next(int i = 1);
    IRType EatOperand();

    // Emit Functions
    void AddLabel(const std::string& label);
    void Emit(std::initializer_list<IRValue> value);
    void Emit(const IRValue& value);

    // Character Parsers
    unsigned char ParseEscape(const std::string& string, size_t& i);
    std::string UnescapeString(const std::string& string);

    // Error Functions
    void Error(const std::string& message);

    // IR Stack Related
    std::optional<int> GetLocal(const std::string& name, bool putError = true);
    void PushLocal(const std::string& name);
    void PushBlock();
    void PopBlock();

    // IR Expr Functions
    void GenPrimary();
    void GenRef();
    void GenMult();
    void GenAdd();

    // IR Stmt Functions
    void GenDecl();
    void GenFunction();
    void GenBlock();
    void GenVarDecl();

    // IR Main Functions
    void GenExpr();
    void GenStmt();
};