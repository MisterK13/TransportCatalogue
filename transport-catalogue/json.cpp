#include "json.h"
using namespace std;

namespace json
{
    namespace
    {
        Node LoadNode(std::istream &input);

        Node LoadNull(std::istream &input)
        {
            const std::string null_str = "null";

            for (char ch : null_str)
            {
                if (input.get() != ch)
                {
                    throw ParsingError("Expected 'null'");
                }
            }

            if (std::isalnum(input.peek()))
            {
                throw ParsingError("Expected end of input after 'null'");
            }

            return Node{nullptr};
        }

        Node LoadBool(std::istream &input)
        {
            const std::string true_str = "true";
            const std::string false_str = "false";

            if (input.peek() == 't')
            {
                for (char ch : true_str)
                {
                    if (input.get() != ch)
                    {
                        throw ParsingError("Expected 'true'");
                    }
                }

                if (std::isalnum(input.peek()))
                {
                    throw ParsingError("Expected end of input after 'true'");
                }

                return Node{true};
            }

            if (input.peek() == 'f')
            {
                for (char ch : false_str)
                {
                    if (input.get() != ch)
                    {
                        throw ParsingError("Expected 'false'");
                    }
                }

                if (std::isalnum(input.peek()))
                {
                    throw ParsingError("Expected end of input after 'false'");
                }

                return Node{false};
            }

            throw ParsingError("Expected 'true' or 'false'");
        }

        Node LoadNumber(std::istream &input)
        {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input]
            {
                parsed_num += static_cast<char>(input.get());
                if (!input)
                {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char]
            {
                if (!std::isdigit(input.peek()))
                {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek()))
                {
                    read_char();
                }
            };

            if (input.peek() == '-')
            {
                read_char();
            }

            // Парсим целую часть числа
            if (input.peek() == '0')
            {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else
            {
                read_digits();
            }

            bool is_int = true;

            // Парсим дробную часть числа
            if (input.peek() == '.')
            {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E')
            {
                read_char();
                if ((ch = input.peek()) == '+' || ch == '-')
                {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try
            {
                if (is_int)
                {
                    // Сначала пробуем преобразовать строку в int
                    try
                    {
                        return Node(std::stoi(parsed_num));
                    }
                    catch (...)
                    {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return Node(std::stod(parsed_num));
            }
            catch (...)
            {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        std::string LoadString(std::istream &input)
        {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;

            while (true)
            {
                if (it == end)
                {
                    // Поток закончился до того, как встретили закрывающую кавычку?
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"')
                {
                    // Встретили закрывающую кавычку
                    ++it;
                    break;
                }
                else if (ch == '\\')
                {
                    // Встретили начало escape-последовательности
                    ++it;
                    if (it == end)
                    {
                        // Поток завершился сразу после символа обратной косой черты
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *it;

                    // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                    switch (escaped_char)
                    {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r')
                {
                    // Строковый литерал внутри JSON не может прерываться символами \r или \n
                    throw ParsingError("Unexpected end of line"s);
                }
                else
                {
                    // Просто считываем очередной символ и помещаем его в результирующую строку
                    s.push_back(ch);
                }
                ++it;
            }

            return s;
        }

        Node LoadArray(std::istream &input)
        {
            Array result;
            if (input.peek() == -1)
                throw ParsingError("Array parsing error");

            for (char c; input >> c && c != ']';)
            {
                if (c != ',')
                {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            return Node{std::move(result)};
        }

        Node LoadDict(std::istream &input)
        {
            Dict result;
            if (input.peek() == -1)
                throw ParsingError("Array parsing error");

            for (char c; input >> c && c != '}';)
            {
                if (c == ',')
                {
                    input >> c; // Пропускаем запятую
                }

                std::string key = LoadString(input);
                input >> c; // Считываем символ после ключа
                result.insert({std::move(key), LoadNode(input)});
            }

            return Node{std::move(result)};
        }

        Node LoadNode(std::istream &input)
        {
            char c;
            input >> c;

            if (c == 'n')
            {
                input.putback(c);
                return LoadNull(input);
            }
            else if (c == 't' || c == 'f')
            {
                input.putback(c);
                return LoadBool(input);
            }
            else if (c == '[')
            {
                return LoadArray(input);
            }
            else if (c == '{')
            {
                return LoadDict(input);
            }
            else if (c == '"')
            {
                return LoadString(input);
            }
            else
            {
                input.putback(c);
                return LoadNumber(input);
            }
        }
    }

    Node::Node(std::nullptr_t)
        : value_(nullptr)
    {
    }

    Node::Node(Array array)
        : value_(std::move(array))
    {
    }

    Node::Node(Dict map)
        : value_(std::move(map))
    {
    }

    Node::Node(bool value)
        : value_(value)
    {
    }

    Node::Node(int value)
        : value_(value)
    {
    }

    Node::Node(double value)
        : value_(value)
    {
    }

    Node::Node(std::string value)
        : value_(std::move(value))
    {
    }

    bool Node::IsInt() const { return std::holds_alternative<int>(value_); }
    bool Node::IsDouble() const { return std::holds_alternative<double>(value_) || std::holds_alternative<int>(value_); }
    bool Node::IsPureDouble() const { return std::holds_alternative<double>(value_); }
    bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }
    bool Node::IsString() const { return std::holds_alternative<std::string>(value_); }
    bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
    bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }
    bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }

    int Node::AsInt() const
    {
        if (!IsInt())
            throw std::logic_error("wrong type");
        return std::get<int>(value_);
    }

    double Node::AsDouble() const
    {
        if (!IsDouble())
            throw std::logic_error("wrong type");
        if (IsInt())
            return static_cast<double>(std::get<int>(value_));
        return std::get<double>(value_);
    }

    bool Node::AsBool() const
    {
        if (!IsBool())
            throw std::logic_error("wrong type");
        return std::get<bool>(value_);
    }

    const std::string &Node::AsString() const
    {
        if (!IsString())
            throw std::logic_error("wrong type");
        return std::get<std::string>(value_);
    }

    const Array &Node::AsArray() const
    {
        if (!IsArray())
            throw std::logic_error("wrong type");
        return std::get<Array>(value_);
    }

    const Dict &Node::AsMap() const
    {
        if (!IsMap())
            throw std::logic_error("wrong type");
        return std::get<Dict>(value_);
    }

    const Node::Value &Node::GetValue() const
    {
        return value_;
    }

    Node::Value &Node::GetValue()
    {
        return value_;
    }

    bool Node::operator==(const Node &rhs) const
    {
        return value_ == rhs.value_;
    }

    bool Node::operator!=(const Node &rhs) const
    {
        return !(value_ == rhs.value_);
    }

    Document::Document(Node root)
        : root_(std::move(root))
    {
    }

    const Node &Document::GetRoot() const
    {
        return root_;
    }

    bool Document::operator==(const Document &rhs) const
    {
        return root_ == rhs.root_;
    }

    bool Document::operator!=(const Document &rhs) const
    {
        return !(root_ == rhs.root_);
    }

    Document Load(std::istream &input)
    {
        return Document{LoadNode(input)};
    }

    template <typename Value>
    void PrintValue(const Value &value, std::ostream &out)
    {
        out << value;
    }

    void PrintValue(std::nullptr_t, std::ostream &out)
    {
        out << "null"sv;
    }

    void PrintValue(bool value, std::ostream &out)
    {
        out << (value ? "true" : "false");
    }

    void PrintValue(const std::string &value, std::ostream &out)
    {
        out << "\""sv;

        for (const char &c : value)
        {
            if (c == '\n')
            {
                out << "\\n"sv;
                continue;
            }
            if (c == '\r')
            {
                out << "\\r"sv;
                continue;
            }
            if (c == '\"')
                out << "\\"sv;
            if (c == '\t')
            {
                out << "\\t"sv;
                continue;
            }
            if (c == '\\')
                out << "\\"sv;
            out << c;
        }

        out << "\""sv;
    }

    void PrintNode(const Node &node, std::ostream &out)
    {
        std::visit(
            [&out](const auto &value)
            { PrintValue(value, out); },
            node.GetValue());
    }

    void PrintValue(const Array &array, std::ostream &out)
    {
        out << '[';
        bool first = true;
        for (const auto &item : array)
        {
            if (!first)
            {
                out << ',';
            }
            first = false;
            PrintNode(item, out);
        }
        out << ']';
    }

    void PrintValue(const Dict &dict, std::ostream &out)
    {
        out << '{';
        bool first = true;
        for (const auto &[key, value] : dict)
        {
            if (!first)
            {
                out << ',';
            }
            first = false;
            PrintValue(key, out);
            out << ':';
            PrintNode(value, out);
        }
        out << '}';
    }

    void Print(const Document &doc, std::ostream &output)
    {
        PrintNode(doc.GetRoot(), output);
    }
} // namespace json
