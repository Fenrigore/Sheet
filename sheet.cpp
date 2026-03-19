#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

#include <unordered_map>
#include <map>
#include <cassert>


#include <sstream>

using namespace std::literals;

struct PositionHasher {
    size_t operator()(const Position& position) const {
        size_t col_hash = std::hash<int>{}(position.col);
        size_t row_hash = std::hash<int>{}(position.row);
        size_t magic_number = 37;
        return col_hash * magic_number + row_hash * magic_number * magic_number;
    }
};

class Sheet::Impl{
public:
    void SetCell(Position pos, std::string text) {
        CheckValidPos(pos);

        // Проверяем, новая ли ячейка
        bool is_new = (sheet_.find(pos) == sheet_.end());

        std::unique_ptr<CellInterface> cur_cell = std::make_unique<Cell>();
        cur_cell->Set(text);
        sheet_[pos] = std::move(cur_cell);

        // Обновляем размер только для новых ячеек
        if (is_new) {
            SetSheetSize(pos);
        }
    }

    void ClearCell(Position pos) {
        CheckValidPos(pos);
        auto it = sheet_.find(pos);
        if (it == sheet_.end()) {
            return;
        }

        //Если ячейка уже пустая — не обновляем размер повторно
        if (it->second->GetText().empty()) {
            return;
        }

        it->second->Set("");
        UpdateSizeAfterClear(pos);
    }

    const CellInterface* GetCell(Position pos) const  {
        //проверяю что позиция валидна
        CheckValidPos(pos);
        auto iter = sheet_.find(pos);
        //если ячейка не найдена или в ней ничего нет
        if (iter == sheet_.end() || iter->second->GetText() == "") {
            return nullptr;
        }
        return iter->second.get();
    }

    CellInterface* GetCell(Position pos)  {
        CheckValidPos(pos);
        auto iter = sheet_.find(pos);
        //если ячейка не найдена или в ней ничего нет
        if (iter == sheet_.end() || iter->second->GetText() == "") {
            return nullptr;
        }
        return iter->second.get();
    }

    Size GetPrintableSize() const {
        return size_;
    }

    //=====================================================================
    std::string EscapeString(const std::string& input) {
        std::ostringstream oss;
        for (char c : input) {
            switch (c) {
            case '\n': oss << "\\n"; break;  // Заменяем перенос на текст \n
            case '\t': oss << "\\t"; break;  // Заменяем табуляцию на текст \t
            case '\r': oss << "\\r"; break;
            case '\\': oss << "\\\\"; break; // Экранируем сам слэш
            default:   oss << c; break;
            }
        }
        return oss.str();
    }
    //=====================================================================

    void PrintTexts(std::ostream& output)  {
        std::ostringstream out_str_str{};

        //прохожу по строке
        for (int i = 0; i < size_.rows; ++i) {
            //по каждой ячейке в строке
            for (int j = 0; j < size_.cols; ++j) {
                auto it = sheet_.find({ i,j });
                if (it != sheet_.end()) {
                    output << it->second->GetText();
                    out_str_str << it->second->GetText();

                }
                if (j != size_.cols - 1) {
                    output << '\t';
                    out_str_str << '\t';
                }
            }
            output << '\n';
            out_str_str << '\n';
        }
        std::string string = out_str_str.str();
        std::string tabul = EscapeString(string);
        //std::cout << '[' << tabul << ']';
    }

    struct ValuePrinter {
        std::string operator()(const std::string& str) {
            return str;
        }

        std::string operator()(double value) {
            std::ostringstream oss;
            oss << std::defaultfloat << value;
            return oss.str();
        }

        std::string operator()(const FormulaError& value) {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    };

    void PrintValues(std::ostream& output)  {

        std::ostringstream out_str_str{};

        for (int i = 0; i < size_.rows; ++i) {
            //по каждой ячейке в строке
            for (int j = 0; j < size_.cols; ++j) {
                auto it = sheet_.find({ i,j });
                if (it != sheet_.end()) {
                    output << std::visit(ValuePrinter{}, it->second->GetValue());
                    out_str_str << std::visit(ValuePrinter{}, it->second->GetValue());

                }
                if (j != size_.cols - 1) {
                    output << '\t';
                    out_str_str << '\t';
                }
            }
            output << '\n';
            out_str_str << '\n';
        }
        std::string string = out_str_str.str();
std::string tabul = EscapeString(string);
//std::cout <<'['<< tabul << ']';
    }

private: //методы
    void CheckValidPos(const Position& pos)const {
        if (!pos.IsValid()) {
            throw InvalidPositionException("Invalid position");
        }
    }

    void SetLineSize(int pos, int& line_size, std::map<int, size_t>& line) {
        //обновляю размер
        line_size = std::max(line_size, pos + 1);
        //тут получается говорю "В такой линии столько то ячеек"
        line[pos + 1] += 1;
    }

    void SetSheetSize(Position pos) {
        SetLineSize(pos.col, size_.cols, count_of_cols_);
        SetLineSize(pos.row, size_.rows, count_of_rows_);
    }

    //Обновить линию (мапа, pos.col или pos.row, size_.col или size_.row)
    int UpdateLine(std::map<int, size_t>& line, int line_pos, int last_size) {
        assert(!line.empty());

        // Ищем запись для позиции (храним как pos + 1)
        auto line_it = line.find(line_pos + 1);
        assert(line_it != line.end());

        // Уменьшаем счетчик
        --line_it->second;

        // если счетчик пуст то удаляем запись
        if (line_it->second <= 0) {
            line.erase(line_it);
        }

        // Если карта пуста, возвращаем 0
        if (line.empty()) {
            return 0;
        }

        // Возвращаем ключ последнего элемента (максимальный размер).
        // Так как мы удалили обнулившиеся записи, этот ключ гарантированно валиден.
        return std::prev(line.end())->first;
    }



    void UpdateSizeAfterClear(Position pos){
        size_.cols = UpdateLine(count_of_cols_, pos.col, size_.cols);
        size_.rows = UpdateLine(count_of_rows_, pos.row, size_.rows);
    }

private: //поля
    //Размер
    Size size_{};
    std::unordered_map<Position, std::unique_ptr<CellInterface>, PositionHasher> sheet_{};
    //Количество столбцов (начиная с 1)
    std::map<int, size_t> count_of_cols_{};
    //Количество строк (начиная с 1)
    std::map<int, size_t> count_of_rows_{};
};

Sheet::Sheet() : impl_(std::make_unique<Impl>()) {}

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    impl_->SetCell(pos, std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return impl_->GetCell(pos);
}
CellInterface* Sheet::GetCell(Position pos) {
    return impl_->GetCell(pos);
}

void Sheet::ClearCell(Position pos) {
    impl_->ClearCell(pos);
}

Size Sheet::GetPrintableSize() const {
    return impl_->GetPrintableSize();
}

void Sheet::PrintValues(std::ostream& output) const {
    impl_->PrintValues(output);
}
void Sheet::PrintTexts(std::ostream& output) const {
    impl_->PrintTexts(output);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}