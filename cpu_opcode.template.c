#include "cpu_opcode.h"
#include "utils.h"

op_profile_t op_profiles[256] = {
/*% profiles.each do |p| -%*/
    [/*%= p.enum %*/] = {
        .op = 0x/*%= p.op %*/,
        .name = {/*%= p.name.downcase.split("").map{|c| "'#{c}'" }.join(",") %*/},
        .official = /*%= p.official %*/,
        .length = /*%= p.length %*/,
        .mode = OP_MODE_/*%= p.mode %*/
    },
/*% end -%*/
};
