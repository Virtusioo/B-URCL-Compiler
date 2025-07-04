
#include "lexer.hpp"
#include "file.hpp"

// Source Functions
void Lexer::Advance()
{
    if (At() == '\n') {
        m_line += 1;
    }
    m_sourceIndex += 1;
}

char Lexer::At() const
{
    return m_source[m_sourceIndex];
}

char Lexer::Eat()
{
    char c = At();
    Advance();
    return c;
}

// Push Function
void Lexer::PushToken(TokenType type, const std::string& value)
{
    m_tokens.emplace_back(Token{type, value, m_line});
}

// Lexer Functions
void Lexer::LexString()
{
    char quote = Eat();
    TokenType type = quote == '\'' ? TokenType::CHAR : TokenType::STRING;

    std::string data;
    char c;

    while ((c = At()) != quote && c != '\0') {
        data += Eat();
    }
    if (Eat() == '\0') {
        PushToken(TokenType::INVALID, "end of file");
        return;
    }
    PushToken(type, data);
}

void Lexer::LexNumber()
{
    std::string number;
    char c;

    while (isdigit(c = At())) {
        number += Eat();
    }
    PushToken(TokenType::NUMBER, number);
}

void Lexer::LexIdent()
{
    TokenType type = TokenType::IDENT;
    std::string ident(1, Eat());
    char c;

    while (isalpha(c = At()) || isdigit(c) || c == '_') {
        ident += Eat();
    }

    if (_identRecord.find(ident) != _identRecord.end()) 
        type = _identRecord[ident];
    
    PushToken(type, ident);
}

// Reserved for Lexer::LexOperand()
#define CaseOperand(operandChar, tokenType) case operandChar: PushToken(tokenType, operand); return

void Lexer::LexOperand()
{
    char c = Eat();
    std::string operand(1, c);

    switch (c) {
        CaseOperand('+', TokenType::PLUS);
        CaseOperand('-', TokenType::MINUS);
        CaseOperand('*', TokenType::STAR);
        CaseOperand('/', TokenType::SLASH);
        CaseOperand('{', TokenType::OPENBRACE);
        CaseOperand('}', TokenType::CLOSEBRACE);
        CaseOperand('(', TokenType::OPENPAREN);
        CaseOperand(')', TokenType::CLOSEPAREN);
        CaseOperand(',', TokenType::COMMA);
        CaseOperand('=', TokenType::EQUAL);
        CaseOperand('&', TokenType::AMPERSAND);
    }
    PushToken(TokenType::INVALID, operand);
}

std::vector<Token> Lexer::Tokenize(const std::string& path) 
{
    m_source = File::ReadEverything(path);
    m_line = 1;
    m_sourceIndex = 0;

    while (m_sourceIndex < m_source.size()) {
        char c = At();
        switch (c) {
            case '\r':
            case ' ':
            case '\n':
                Advance();
                break;
            case '\'':
            case '"':
                LexString();
                break;
            default: {
                if (isalpha(c) || c == '_') {
                    LexIdent();
                    break;
                } 
                if (isdigit(c)) {
                    LexNumber();
                    break;
                }
                LexOperand();
                break;
            }
        }
    }

    PushToken(TokenType::END_OF_FILE, "end of file");
    
    return std::move(m_tokens);
}