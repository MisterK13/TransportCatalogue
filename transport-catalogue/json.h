#pragma once

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <cctype>

namespace json
{

    class Node;

    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    class ParsingError : public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };

    class Node final : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>
    {
    public:
        using variant::variant;
        using Value = variant;
        Node(Value value) : variant(std::move(value)) {};

        Node() = default;
        Node(std::nullptr_t);
        Node(int value);
        Node(double value);
        Node(bool value);
        Node(std::string value);
        Node(Array array);
        Node(Dict map);

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        double AsDouble() const;
        bool AsBool() const;
        const std::string &AsString() const;
        const Array &AsArray() const;
        const Dict &AsMap() const;

        const Value &GetValue() const;
        Value &GetValue();

        bool operator==(const Node &rhs) const;
        bool operator!=(const Node &rhs) const;

    private:
        Value value_;
    };

    class Document
    {
    public:
        explicit Document(Node root);
        const Node &GetRoot() const;

        bool operator==(const Document &rhs) const;
        bool operator!=(const Document &rhs) const;

    private:
        Node root_;
    };

    Document Load(std::istream &input);

    // Контекст вывода, хранит ссылку на поток вывода и текущий отступ
    struct PrintContext
    {
        std::ostream &out;
        int indent_step = 4;
        int indent = 0;

        void PrintIndent() const
        {
            for (int i = 0; i < indent; ++i)
            {
                out.put(' ');
            }
        }

        // Возвращает новый контекст вывода с увеличенным смещением
        PrintContext Indented() const
        {
            return {out, indent_step, indent + indent_step};
        }
    };

    void Print(const Document &doc, std::ostream &output);

} // namespace json
