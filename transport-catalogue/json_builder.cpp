#include "json_builder.h"

namespace json
{
    Builder::Builder() : root_(), nodes_stack_{&root_} {}

    Builder::DictValueContext Builder::Key(std::string key)
    {
        if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap() || current_key_)
            throw std::logic_error("Error calling Key()");

        current_key_ = std::move(key);

        return BaseContext{*this};
    }

    Builder::BaseContext Builder::Value(Node::Value value)
    {
        const auto value_v = value;
        Node *node_back = AddNode(current_key_, std::move(value));
        node_back->GetValue() = std::move(value_v);
        return *this;
    }

    Builder::DictItemContext Builder::StartDict()
    {
        Node *node_back = AddNode(current_key_, Dict{});
        nodes_stack_.emplace_back(node_back);
        return BaseContext{*this};
    }

    Builder::ArrayItemContext Builder::StartArray()
    {
        Node *node_back = AddNode(current_key_, Array{});
        nodes_stack_.emplace_back(node_back);
        return BaseContext{*this};
    }

    Builder &Builder::EndDict()
    {
        if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap())
        {
            throw std::logic_error("Error calling EndDict()");
        }
        nodes_stack_.pop_back();
        return *this;
    }

    Builder &Builder::EndArray()
    {
        if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray())
        {
            throw std::logic_error("Error calling EndArray()");
        }

        nodes_stack_.pop_back();
        return *this;
    }

    Node Builder::Build()
    {
        if (root_.IsNull() || nodes_stack_.size() > 1)
        {
            throw std::logic_error("Error calling Build()");
        }
        return std::move(root_);
    }

    Node::Value &Builder::GetCurrentValue()
    {
        if (nodes_stack_.empty())
        {
            throw std::logic_error("Attempt to change finalized JSON");
        }
        return nodes_stack_.back()->GetValue();
    }

    const Node::Value &Builder::GetCurrentValue() const
    {
        return const_cast<Builder *>(this)->GetCurrentValue();
    }

    Node *Builder::AddNode(std::optional<std::string> &current_key, Node::Value value)
    {
        Node::Value &node_back_value = GetCurrentValue();
        if (std::holds_alternative<Dict>(node_back_value))
        {
            if (!current_key)
                throw std::logic_error("Error: the key is missing");

            auto &dict = std::get<Dict>(node_back_value);
            const std::string old_key = current_key.value();
            dict.emplace(std::move(current_key.value()), std::move(value));
            current_key = std::nullopt;
            return &dict.at(old_key);
        }
        else if (std::holds_alternative<Array>(node_back_value))
        {
            auto &array = std::get<Array>(node_back_value);
            array.emplace_back(std::move(value));
            return &array.back();
        }
        else if (nodes_stack_.back()->IsNull())
        {
            node_back_value = std::move(value);
            return nodes_stack_.back();
        }
        else
        {
            throw std::logic_error("Error: invalid operation");
        }
    }
}