
#ifndef vShellEngineCOMMANDPARSER_HPP
#define vShellEngineCOMMANDPARSER_HPP

#include <string>
#include <vector>

#include "vKeyWords.hpp"
#include "ConsoleManager.hpp"

class LiteralNode : public ASTNode {
    vDataValue value;

public:
    explicit LiteralNode(vDataValue v) : value(std::move(v)) {}

    vDataValue evaluate(vShellEngine& engine) override {
        return value; // Returnează valoarea direct
    }
};

class VariableNode : public ASTNode {
    std::wstring varName;

public:
    explicit VariableNode(std::wstring name) : varName(std::move(name)) {}

    vDataValue evaluate(vShellEngine& engine) override {
        // Folosim metoda ta existentă pentru a extrage valoarea din map
        return engine.getVarValue(varName);
    }
};


class ASTNode {
public:
    virtual ~ASTNode() = default;
    // Contextul este motorul tău de shell, pentru a accesa m_variables
    virtual vDataValue evaluate(vShellEngine& engine) = 0;
};


class BinaryOpNode : public ASTNode {
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::wstring op; // "+", "-", "*", "/", "==", "&&" etc.

public:
    BinaryOpNode(std::unique_ptr<ASTNode> l, std::wstring o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), op(o), right(std::move(r)) {
    }

    vDataValue evaluate(vShellEngine& engine) override {
        // Pasul 1: Evaluăm recursiv operanzii (mergem în jos pe arbore)
        vDataValue leftVal = left->evaluate(engine);
        vDataValue rightVal = right->evaluate(engine);

        // Pasul 2: Aplicăm logica în funcție de operator
        if (op == L"+") return performAddition(leftVal, rightVal);
        if (op == L"*") return performMultiplication(leftVal, rightVal);
        if (op == L"==") return (leftVal == rightVal); // Variant suportă deja ==

        return std::monostate{};
    }

private:
    // Aici gestionăm tipurile dinamice (Int + Int, Float + Int, String + String)
    vDataValue performAddition(const vDataValue& l, const vDataValue& r) {
        // Cazul: Ambele sunt Long Long
        if (std::holds_alternative<long long>(l) && std::holds_alternative<long long>(r)) {
            return std::get<long long>(l) + std::get<long long>(r);
        }
        // Cazul: Cel puțin unul e Double (promovăm la Double)
        if (isNumeric(l) && isNumeric(r)) {
            return asDouble(l) + asDouble(r);
        }
        // Cazul: Concatenare de String-uri
        if (std::holds_alternative<std::wstring>(l) || std::holds_alternative<std::wstring>(r)) {
            return engine.vValueToString(l) + engine.vValueToString(r);
        }
        return std::monostate{};
    }
};


class vExpressionParser {
    std::vector<std::wstring> tokens;
    size_t pos = 0;

public:
    std::unique_ptr<ASTNode> parse() {
        return parseExpression(); // Începe cu cel mai mic nivel de precedență (OR/AND)
    }

private:
    // Nivel 1: Adunare și Scădere
    std::unique_ptr<ASTNode> parseAddition() {
        auto left = parseMultiplication();
        while (match(L"+") || match(L"-")) {
            std::wstring op = prev();
            auto right = parseMultiplication();
            left = std::make_unique<BinaryOpNode>(std::move(left), op, std::move(right));
        }
        return left;
    }

    // Nivel 2: Înmulțire și Împărțire
    std::unique_ptr<ASTNode> parseMultiplication() {
        auto left = parsePrimary();
        // ... logică similară pentru * și /
        return left;
    }

    // Nivel 3: Valori atomice (Cea mai mare precedență)
    std::unique_ptr<ASTNode> parsePrimary() {
        if (match(L"(")) {
            auto node = parseExpression();
            consume(L")", L"Expect ')' after expression.");
            return node;
        }
        // Dacă e variabilă, returnează VariableNode
        // Dacă e număr/string, returnează LiteralNode
    }
};


struct ShellCommand {
    std::wstring name;
    std::vector<std::wstring> args;
    bool isValid = false;
};

class vShellEngineCommandParser {
private:

    // --- TOKENIZER ---
    static std::vector<std::wstring> tokenize(const std::wstring& line) {
        std::vector<std::wstring> tokens;
        std::wstring tok;
        bool inQuotes = false;

        auto flush = [&]() {
            if (!tok.empty()) {
                tokens.push_back(tok);
                tok.clear();
            }
            };

        for (size_t i = 0; i < line.size(); ++i) {
            wchar_t c = line[i];

            // toggle quotes
            if (c == L'"') {
                inQuotes = !inQuotes;
                continue;
            }

            if (!inQuotes) {
                // operators of 2 chars
                if (i + 1 < line.size()) {
                    std::wstring two = line.substr(i, 2);
                    if (two == L"==" || two == L"!=" || two == L">=" || two == L"<=" ||
                        two == L"&&" || two == L"||" || two == L">>") {
                        flush();
                        tokens.push_back(two);
                        i++;
                        continue;
                    }
                }

                // operators of 1 char
                if (wcschr(L"=+-*<>|;()", c)) {
                    flush();
                    tokens.push_back(std::wstring(1, c));
                    continue;
                }

                // whitespace
                if (iswspace(c)) {
                    flush();
                    continue;
                }
            }

            tok += c;
        }

        flush();
        return tokens;
    }

public:

    // --- PARSER ---
    static ShellCommand parse(const std::wstring& line) {
        ShellCommand cmd;

        if (line.empty() || line[0] != L'/')
            return cmd;

        auto tokens = tokenize(line);

        if (tokens.empty())
            return cmd;

        cmd.name = tokens[0];

        // validate command
        if (!vKeyWords::isShellCommand(cmd.name)) {
            LOG_ERROR(L"Unknown command: " + cmd.name);
            return cmd;
        }

        // rest are args
        for (size_t i = 1; i < tokens.size(); ++i)
            cmd.args.push_back(tokens[i]);

        cmd.isValid = true;
        return cmd;
    }
};

#endif



// 1. Construim frunzele
auto left = std::make_unique<VariableNode>(L"$a");
auto right = std::make_unique<LiteralNode>(10LL); // 10 ca long long

// 2. Construim nodul de operație (Arborele)
auto root = std::make_unique<BinaryOpNode>(std::move(left), L"+", std::move(right));

// 3. Executăm!
vDataValue finalResult = root->evaluate(engine);

/*

#ifndef vShellEngineCOMMANDPARSER_HPP
#define vShellEngineCOMMANDPARSER_HPP

#include <string>
#include <vector>
#include <sstream>

#include "vKeyWords.hpp"
#include "ConsoleManager.hpp"

struct ShellCommand {
    std::wstring name;               // Ex: L"/imp"
    std::vector<std::wstring> args;   // [L"dbf", L"C:\My Data\file.dbf", L"public.table"]
    bool isValid = false;
};

class vShellEngineCommandParser {
public:
    static ShellCommand parse(const std::wstring& line) {
        ShellCommand cmd;

        if (line.empty() || line[0] != L'/') return cmd;

        if (cmd.isValid) {
            if (!vKeyWords::isShellCommand(cmd.name)) {
                LOG_ERROR(L"Unknown command: " + cmd.name);
                cmd.isValid = false;
            }
        }
       

        std::wstring currentToken;
        bool inQuotes = false;
        
        for (size_t i = 0; i < line.length(); ++i) {
            wchar_t c = line[i];

            if (c == L'\"') {
                inQuotes = !inQuotes;
                currentToken += c; // ADĂUGĂM ghilimeaua, nu o sărim!
                continue;
            }

            if (iswspace(c) && !inQuotes) { // iswspace e mai sigur decât L' '
                if (!currentToken.empty()) {
                    if (cmd.name.empty()) cmd.name = currentToken;
                    else cmd.args.push_back(currentToken);
                    currentToken.clear();
                }
            }
            else {
                currentToken += c;
            }
        }
        // Adăugăm ultimul token dacă există
        if (!currentToken.empty()) {
            if (cmd.name.empty()) cmd.name = currentToken;
            else cmd.args.push_back(currentToken);
        }

        cmd.isValid = !cmd.name.empty();
        return cmd;
    }

    

};

#endif
*/