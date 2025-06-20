#pragma once

#include "json.h"

#include <vector>
#include <stdexcept>
#include <optional>

namespace json
{
    class Builder
    {
    private:
        class BaseContext;
        class DictItemContext;
        class DictValueContext;
        class ArrayItemContext;

    public:
        Builder();
        DictValueContext Key(std::string key);
        BaseContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        Builder &EndDict();
        Builder &EndArray();
        Node Build();
        Node *AddNode(std::optional<std::string> &current_key, Node::Value value);

    private:
        Node root_;
        std::vector<Node *> nodes_stack_;
        std::optional<std::string> current_key_{std::nullopt};

        Node::Value &GetCurrentValue();
        const Node::Value &GetCurrentValue() const;

        class BaseContext
        {
        public:
            BaseContext(Builder &builder) : builder_(builder) {}
            Node Build() { return builder_.Build(); }
            DictValueContext Key(std::string key) { return builder_.Key(std::move(key)); }
            BaseContext Value(Node::Value value) { return builder_.Value(std::move(value)); }
            DictItemContext StartDict() { return builder_.StartDict(); }
            ArrayItemContext StartArray() { return builder_.StartArray(); }
            BaseContext EndDict() { return builder_.EndDict(); }
            BaseContext EndArray() { return builder_.EndArray(); }

        private:
            Builder &builder_;
        };

        class DictItemContext : public BaseContext
        {
        public:
            DictItemContext(BaseContext base) : BaseContext(base) {}
            Node Build() = delete;
            BaseContext Value(Node::Value value) = delete;
            BaseContext EndArray() = delete;
            DictItemContext StartDict() = delete;
            ArrayItemContext StartArray() = delete;
        };

        class ArrayItemContext : public BaseContext
        {
        public:
            ArrayItemContext(BaseContext base) : BaseContext(base) {}
            ArrayItemContext Value(Node::Value value) { return BaseContext::Value(std::move(value)); }
            Node Build() = delete;
            DictValueContext Key(std::string key) = delete;
            BaseContext EndDict() = delete;
        };

        class DictValueContext : public BaseContext
        {
        public:
            DictValueContext(BaseContext base) : BaseContext(base) {}
            DictItemContext Value(Node::Value value) { return BaseContext::Value(std::move(value)); }
            Node Build() = delete;
            DictValueContext Key(std::string key) = delete;
            BaseContext EndDict() = delete;
            BaseContext EndArray() = delete;
        };
    };

}