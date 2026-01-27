#ifndef INSTRUCTION_CHECKING_H
#define INSTRUCTION_CHECKING_H

#include "source_file_token.h"
#include "structure_builder.h"
#include "translator_error_type.h"

class instruction_check {
public:
    virtual bool is_match(source_file_token instr) = 0;
    virtual void check_errors(const structure_builder::instruction& instr, std::vector<translator_error_type>& err_list) = 0;
    virtual ~instruction_check() = default;
};

template<typename filter_type>
class generic_instruction_check : public instruction_check {
private:
    std::vector<source_file_token> instruction_list;
    filter_type filter;

public:
    generic_instruction_check(std::vector<source_file_token>&& list, filter_type built_filter = filter_type{})
        :instruction_list{ std::move(list) },
        filter{ built_filter }
    {}

    virtual bool is_match(source_file_token instr) override {
        return std::ranges::find(this->instruction_list, instr) != this->instruction_list.end();
    }

    virtual void check_errors(const structure_builder::instruction& instr, std::vector<translator_error_type>& err_list) override {
        if (!this->filter.check(instr)) {
            err_list.push_back(this->filter.error_message);
        }
    }
};

#endif
