#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg
{
    struct Rgb
    {
        Rgb() = default;

        Rgb(uint8_t red_v, uint8_t green_v, uint8_t blue_v) : red(red_v), green(green_v), blue(blue_v) {}

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba : public Rgb
    {
        Rgba() : Rgb(), opacity(1.0) {}

        Rgba(uint8_t red_v, uint8_t green_v, uint8_t blue_v, double opacity_v) : Rgb(red_v, green_v, blue_v), opacity(opacity_v) {}

        double opacity;
    };

    using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
    inline const Color NoneColor{"none"};

    enum class StrokeLineCap
    {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin
    {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream &operator<<(std::ostream &out, Color &color);
    std::ostream &operator<<(std::ostream &out, StrokeLineCap line_cap);
    std::ostream &operator<<(std::ostream &out, StrokeLineJoin line_join);

    struct ColorPrinter
    {
        std::ostream &out;

        void operator()(std::monostate) const
        {
            out << "none";
        }
        void operator()(std::string color) const
        {
            out << color;
        }
        void operator()(Rgb color) const
        {
            out << "rgb(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << ","
                << static_cast<int>(color.blue) << ")";
        }
        void operator()(Rgba color) const
        {
            out << "rgba(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << ","
                << static_cast<int>(color.blue) << "," << static_cast<double>(color.opacity) << ")";
        }
    };

    struct Point
    {
        Point() = default;
        Point(double x_p, double y_p)
            : x(x_p), y(y_p)
        {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext
    {
        RenderContext(std::ostream &out_o)
            : out(out_o)
        {
        }

        RenderContext(std::ostream &out_o, int indent_step_o, int indent_o = 0)
            : out(out_o), indent_step(indent_step_o), indent(indent_o)
        {
        }

        RenderContext Indented() const
        {
            return {out, indent_step, indent + indent_step};
        }

        void RenderIndent() const
        {
            for (int i = 0; i < indent; ++i)
            {
                out.put(' ');
            }
        }

        std::ostream &out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object
    {
    public:
        void Render(const RenderContext &context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext &context) const = 0;
    };

    class ObjectContainer
    {
    public:
        template <typename Obj>
        void Add(Obj obj)
        {
            AddPtr(std::make_unique<Obj>(std::move(obj)));
        }

        virtual void AddPtr(std::unique_ptr<Object> &&obj) = 0;

    protected:
        ~ObjectContainer() = default;
    };

    class Drawable
    {
    public:
        virtual ~Drawable() = default;
        virtual void Draw(ObjectContainer &container) const = 0;
    };

    template <typename Owner>
    class PathProps
    {
    public:
        Owner &SetFillColor(Color color)
        {
            fill_color_ = std::move(color);
            return AsOwner();
        }

        Owner &SetStrokeColor(Color color)
        {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner &SetStrokeWidth(double width)
        {
            width_ = std::move(width);
            return AsOwner();
        }

        Owner &SetStrokeLineCap(StrokeLineCap line_cap)
        {
            line_cap_ = line_cap;
            return AsOwner();
        }

        Owner &SetStrokeLineJoin(StrokeLineJoin line_join)
        {
            line_join_ = line_join;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        // Метод RenderAttrs выводит в поток общие для всех путей атрибуты fill и stroke
        void RenderAttrs(std::ostream &out) const
        {
            using namespace std::literals;

            if (fill_color_)
            {
                out << " fill=\""sv;
                std::visit(ColorPrinter{out}, *fill_color_);
                out << "\""sv;
            }
            if (stroke_color_)
            {
                out << " stroke=\""sv;
                std::visit(ColorPrinter{out}, *stroke_color_);
                out << "\""sv;
            }
            if (width_)
            {
                out << " stroke-width=\""sv << *width_ << "\""sv;
            }
            if (line_cap_)
            {
                out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
            }
            if (line_join_)
            {
                out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
            }
        }

    private:
        Owner &AsOwner()
        {
            // static_cast безопасно преобразует *this к Owner&,
            // если класс Owner — наследник PathProps
            return static_cast<Owner &>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> width_;
        std::optional<StrokeLineCap> line_cap_;
        std::optional<StrokeLineJoin> line_join_;
    };

    /*
     * Класс Circle моделирует элемент <circle> для отображения круга
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle>
    {
    public:
        Circle &SetCenter(Point center);
        Circle &SetRadius(double radius);

    private:
        void RenderObject(const RenderContext &context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline>
    {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline &AddPoint(Point point);

        /*
         * Прочие методы и данные, необходимые для реализации элемента <polyline>
         */
    private:
        void RenderObject(const RenderContext &context) const override;

        std::vector<Point> points_;
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text>
    {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text &SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text &SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text &SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text &SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text &SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text &SetData(std::string data);

        // Прочие данные и методы, необходимые для реализации элемента <text>
    private:
        void RenderObject(const RenderContext &context) const override;

        Point pos_ = {0.0, 0.0};
        Point offset_ = {0.0, 0.0};
        uint32_t size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class Document : public ObjectContainer
    {
    public:
        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object> &&obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream &out) const;

    private:
        std::vector<std::unique_ptr<Object>> objects_;
    };

} // namespace svg