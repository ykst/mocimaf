#include "disas_driver_impl.h"
#include "disas.h"

typedef struct disas_driver {
    disas_ref disas;
    FILE *out_fp;
    char *out_path;
} disas_driver_t;

void disas_driver_destroy(disas_driver_ref self)
{
    if (self) {
        if (self->out_fp) {
            fclose(self->out_fp);
        }
        FREE(self->out_path);
        FREE(self);
    }
}

disas_driver_ref disas_driver_create(core_ref core, const char *out_path)
{
    disas_driver_ref self = NULL;
    TALLOC(self);

    GUARD(out_path);
    GUARD(self->out_path = strdup(out_path));
    GUARD(self->out_fp = fopen(self->out_path, "wx"));

    GUARD(self->disas = disas_create(core->mapper));

    return self;
error:
    disas_driver_destroy(self);
    return NULL;
}

void disas_driver_on_cpu_decode(disas_driver_ref self, cpu_ref cpu, uint16_t pc)
{
    disas_mark_op_runtime(self->disas, cpu->reg.pc, cpu->last_op == OP_20_JSR);
}

void disas_driver_on_core_start(disas_driver_ref self, core_ref core)
{
    disas_mark_from_vectors(self->disas);
}

void disas_driver_on_core_quit(disas_driver_ref self, core_ref core)
{
    disas_dump(self->disas, self->out_fp);
}

void disas_driver_on_cpu_read(disas_driver_ref self, cpu_ref cpu, uint16_t addr,
                              size_t length)
{
    if (addr >= 0x8000) {
        disas_mark_data_runtime(self->disas, addr, length);
    }
}
