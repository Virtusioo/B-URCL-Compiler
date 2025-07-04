
#include "ir.hpp"

#include <string>
#include <iomanip>
#include <iostream>

// Character Parsers
unsigned char IRGenerator::ParseEscape(const std::string& s, size_t& i) 
{
    if (s[i] != '\\') 
        return s[i];

    i += 1;

    if (i >= s.size()) {
        Error("incomplete escape sequence");
        return 0;
    }

    switch (s[i]) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '0': return '\0';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '\"';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'v': return '\v';
        case 'a': return '\a';

        case 'x': { 
            if (i + 2 >= s.size()) {
                Error("incomplete hex escape");
                return 0;
            }
            std::string hex = s.substr(i + 1, 2);
            i += 2;
            return static_cast<char>(std::stoi(hex, nullptr, 16));
        }

        case 'u': { 
            if (i + 4 >= s.size()) {
                Error("incomplete unicode escape");
                return 0;
            }
            std::string hex = s.substr(i + 1, 4);
            i += 4;
            return static_cast<char>(std::stoi(hex, nullptr, 16)); // UTF-8 simplified
        }

        default:
            Error("unknown escape sequence: \\" + std::string(1, s[i]));
            return 0;
    }
}

std::string IRGenerator::UnescapeString(const std::string& input) 
{
    std::string output;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\') {
            output += ParseEscape(input, i);
        } else {
            output += input[i];
        }
    }
    return output;
}

// Source Functions
Token& IRGenerator::At()
{
    return m_tokens[m_tokenIndex];
}

Token& IRGenerator::Eat()
{
    Token& t = At();
    Advance();
    return t;
}

TokenType IRGenerator::Type()
{
    return At().type;
}

void IRGenerator::Advance()
{
    if (Type() == TokenType::END_OF_FILE)
        return;
    m_tokenIndex += 1;
}

Token& IRGenerator::Next(int i)
{
    return m_tokens[(int)m_tokenIndex + i];
}

Token& IRGenerator::Expect(TokenType type, const std::string& messsage)
{
    Token& t = Eat();
    if (t.type != type) {
        Error(messsage+", got '"+t.value+"'");
    }
    return t;
}

// Reserved for IRGenerator::EatOperand()
#define CaseOperand(tokenType, irType) case tokenType: return irType

IRType IRGenerator::EatOperand()
{
    Token& t = Eat();
    switch (t.type) {
        CaseOperand(TokenType::PLUS, IRType::ADD);
        CaseOperand(TokenType::MINUS, IRType::SUB);
        CaseOperand(TokenType::STAR, IRType::MUL);
        CaseOperand(TokenType::SLASH, IRType::DIV);
    }
    return (IRType)0;
}

// Emit Functions
void IRGenerator::AddLabel(const std::string& label)
{
    m_irInfo.labels[label] = IRValues();
    m_currentLabel = label;
}
void IRGenerator::Emit(std::initializer_list<IRValue> value)
{
    for (IRValue v: value) {
        if (m_currentLabel == "?") 
            continue;
        m_irInfo.labels[m_currentLabel].emplace_back(v);
    }
}

void IRGenerator::Emit(const IRValue& value)
{
    if (m_currentLabel == "?") 
        return;
    m_irInfo.labels[m_currentLabel].emplace_back(value);
}

// IR Stack Related
std::optional<int> IRGenerator::GetLocal(const std::string& name, bool putError)
{
    for (int i = (int)m_locals.size() - 1; i >= 0; --i) {
        auto& scope = m_locals[i];
        if (scope.find(name) != scope.end()) {
            return -scope[name];
        }
    }
    if (m_params.find(name) != m_params.end()) {
        return m_params[name];
    }
    if (putError)
        Error("local '" + name + "' does not exist");
    return {};
}


void IRGenerator::PushLocal(const std::string& name)
{
    auto& scope = m_locals.back();
    scope[name] = m_localsSize++;
}

void IRGenerator::PushBlock()
{
    m_localsSizeStack.push_back(m_localsSize);
    m_locals.emplace_back();
}

void IRGenerator::PopBlock()
{
    m_localsSize = m_localsSizeStack.back();
    m_locals.pop_back();
    m_localsSizeStack.pop_back();
}

// IR  Expr Functions
void IRGenerator::GenPrimary()
{
    Token& t = Eat();

    switch (t.type) {
        case TokenType::NUMBER: {
            Emit({IRType::LOAD_NUMBER, t.value}); 
            break;
        }
        case TokenType::STRING: {
            size_t last = m_irInfo.stringPtr;
            std::string string = UnescapeString(t.value);
            m_irInfo.strings.emplace_back(string); 
            m_irInfo.stringPtr += t.value.size();
            Emit({IRType::LOAD_NUMBER, std::to_string(last)});
            break;
        }
        case TokenType::CHAR: {
            size_t i = 0;
            Emit({IRType::LOAD_NUMBER, std::to_string(ParseEscape(t.value, i))});
            break;
        }
        case TokenType::IDENT: {
            auto local = GetLocal(t.value);
            if (!local.has_value()) break;
            Emit({IRType::LOAD_STACK, std::to_string(local.value())});
            break;
        }
        default: {
            Error("unexpected symbol '"+t.value+"'");
            break;
        }
    }
}

void IRGenerator::GenRef()
{
    if (Type() == TokenType::STAR) {
        while (Type() == TokenType::STAR) {
            Advance();
            GenPrimary();
            Emit(IRType::DEREF);
        } 
    } else if (Type() == TokenType::AMPERSAND) {
        Advance();
        Token& value = Expect(TokenType::IDENT, "expected lvalue next to address-of operator");
        if (value.type == TokenType::IDENT) {
            auto local = GetLocal(value.value);
            if (local.has_value()) {
                Emit({IRType::LOAD_STACK, std::to_string(local.value())});
            }
        }
    } else {
        GenPrimary();
    }
}

void IRGenerator::GenMult()
{
    GenPrimary();
    while (Type() == TokenType::STAR || Type() == TokenType::SLASH) {
        IRType op = EatOperand();
        GenPrimary();
        Emit(op);
    }
}

void IRGenerator::GenAdd()
{
    GenMult();
    while (Type() == TokenType::PLUS || Type() == TokenType::MINUS) {
        IRType op = EatOperand();
        GenMult();
        Emit(op);
    } 
}

// IR Stmt Functions
void IRGenerator::GenFunction() 
{
    AddLabel(Eat().value);
    Advance();
    m_params.clear();

    size_t param = 1;
    std::vector<std::string> params;

    if (Type() != TokenType::CLOSEPAREN) {
        Token& t = Expect(TokenType::IDENT, "invalid parameter #1");
        if (t.type == TokenType::IDENT) 
            params.push_back(t.value);
        param += 1;
    }

    while (Type() != TokenType::CLOSEPAREN && Type() != TokenType::END_OF_FILE) {
        Expect(TokenType::COMMA, "expected ',' beside paremeter #"+std::to_string(param-1));

        Token& t = Expect(TokenType::IDENT, "invalid parameter #"+std::to_string(param));
        if (t.type == TokenType::IDENT) 
            params.push_back(t.value);
        param += 1;
    }

    Expect(TokenType::CLOSEPAREN, "expected ')' when closing parameters");
    if (params.size() != param-1) 
        return;

    for (auto& p: params) {
        m_params[p] = --param;
    }

    GenBlock();
}

void IRGenerator::GenBlock()
{
    Expect(TokenType::OPENBRACE, "expected '{' when opening scope");
    PushBlock();
    while (Type() != TokenType::CLOSEBRACE && Type() != TokenType::END_OF_FILE) {
        GenStmt();
    }
    PopBlock();
    Expect(TokenType::CLOSEBRACE, "expected '}' when closing scope");
}

void IRGenerator::GenVarDecl()
{
    const std::string& var = Eat().value;
    auto local = GetLocal(var, false);
    Advance();
    if (local.has_value()) {
        GenExpr();
        Emit({IRType::ASSIGN, std::to_string(local.value())});
    } else {
        PushLocal(var);
        GenExpr();
    }
}

void IRGenerator::GenDecl()
{
    const Token& next = Next();

    if (next.type == TokenType::OPENPAREN) {
        GenFunction();
    } else if (next.type == TokenType::EQUAL) {
        GenVarDecl();
    } else {
        Advance();
        Error("expected declaration");
    }
}

// IR Main Functions
void IRGenerator::GenExpr()
{
    GenAdd();
}

void IRGenerator::GenStmt()
{
    switch (Type()) {
        case TokenType::IDENT: 
            return GenDecl();
    }
    Error("expected declaration");
    GenExpr();
}

// Error Stuff
void IRGenerator::Error(const std::string& message) 
{
    Token& t = m_tokenIndex == 0 ? At() : Next(-1);
    m_gotError = true;
    m_errors << "[ERROR]: " << m_currentLabel << ": " << m_sourceName << ':' << t.line << ": " << message << '\n';
}

bool IRGenerator::PrintErrors()
{
    if (m_gotError)
        std::cout << m_errors.str();
    return m_gotError;
}

void IRGenerator::SetSourceName(const std::string& name)
{
    m_sourceName = name;
}

// Main
IRInfo IRGenerator::Generate(const std::vector<Token>& tokens)
{
    m_tokens = tokens;
    m_tokenIndex = 0;
    m_irInfo.stringPtr = 0;
    m_errors.clear();
    m_gotError = false;
    m_currentLabel = "?";
    m_localsSize = 0;

    while (Type() != TokenType::END_OF_FILE) {
        GenStmt();
    }

    return std::move(m_irInfo);
}