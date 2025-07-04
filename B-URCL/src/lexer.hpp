
#pragma once

#include <vector>
#include <string>
#include <unordered_map>

enum class TokenType
{
    // Basic Types
    NUMBER,
    STRING,
    IDENT,
    CHAR,
    INVALID,
    END_OF_FILE,
    // Identifier Record Types
    RETURN,
    AUTO,
    EXTRN,
    // Independent Types
    PLUS,
    MINUS,
    STAR,
    SLASH,
    EQUAL,
    COMMA,
    AMPERSAND,
    SEMICOLON,
    OPENPAREN,
    CLOSEPAREN,
    OPENBRACE,
    CLOSEBRACE
};

struct Token
{
    TokenType type;
    std::string value;
    size_t line, col;
};

static std::unordered_map<std::string, TokenType> _identRecord = {
    {"return", TokenType::RETURN},
    {"auto", TokenType::AUTO},
    {"extrn", TokenType::EXTRN}
};

class Lexer 
{
public:
    std::vector<Token> Tokenize(const std::string& path);

private:
    // Lexer Info
    std::vector<Token> m_tokens;
    std::string m_source;
    size_t m_sourceIndex;
    size_t m_line;

    // Source Functions
    void Advance();
    char At() const;
    char Eat();

    // Push Function
    void PushToken(TokenType type, const std::string& value);

    // Lexer Functions
    void LexString();
    void LexNumber();
    void LexIdent();
    void LexOperand();
};