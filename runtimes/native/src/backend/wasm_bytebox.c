#include <bytebox.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "../wasm.h"
#include "../runtime.h"

void* wasm_mem;

bb_module_definition* module_def;
bb_module_instance* module_inst;
bb_import_package* wasm_package;

bb_func_handle start;
bb_func_handle update;

typedef char byte_t;

static void* getMemoryPointer (const bb_val* val) {
    int32_t offset = val->i32_val;
    byte_t* data = (byte_t*)bb_module_instance_mem(module_inst, offset, 1);
    return (offset < 0 || offset >= (1 << 16)) ? NULL : (void*)(data);
}

static void blit (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint8_t* sprite = getMemoryPointer(&params[0]);
    int32_t x = params[1].i32_val;
    int32_t y = params[2].i32_val;
    int32_t width = params[3].i32_val;
    int32_t height = params[4].i32_val;
    int32_t flags = params[5].i32_val;
    w4_runtimeBlit(sprite, x, y, width, height, flags);
}

static void blitSub (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint8_t* sprite = getMemoryPointer(&params[0]);
    int32_t x = params[1].i32_val;
    int32_t y = params[2].i32_val;
    int32_t width = params[3].i32_val;
    int32_t height = params[4].i32_val;
    int32_t srcX = params[5].i32_val;
    int32_t srcY = params[6].i32_val;
    int32_t stride = params[7].i32_val;
    int32_t flags = params[8].i32_val;
    w4_runtimeBlitSub(sprite, x, y, width, height, srcX, srcY, stride, flags);
}

static void line (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    int32_t x1 = params[0].i32_val;
    int32_t y1 = params[1].i32_val;
    int32_t x2 = params[2].i32_val;
    int32_t y2 = params[3].i32_val;
    w4_runtimeLine(x1, y1, x2, y2);
}

static void hline (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    int32_t x = params[0].i32_val;
    int32_t y = params[1].i32_val;
    int32_t len = params[2].i32_val;
    w4_runtimeHLine(x, y, len);
}

static void vline (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    int32_t x = params[0].i32_val;
    int32_t y = params[1].i32_val;
    int32_t len = params[2].i32_val;
    w4_runtimeVLine(x, y, len);
}

static void oval (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    int32_t x = params[0].i32_val;
    int32_t y = params[1].i32_val;
    int32_t width = params[2].i32_val;
    int32_t height = params[3].i32_val;
    w4_runtimeOval(x, y, width, height);
}

static void rect (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    int32_t x = params[0].i32_val;
    int32_t y = params[1].i32_val;
    int32_t width = params[2].i32_val;
    int32_t height = params[3].i32_val;
    w4_runtimeRect(x, y, width, height);
}

static void text (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const char* str = getMemoryPointer(&params[0]);
    int32_t x = params[1].i32_val;
    int32_t y = params[2].i32_val;
    w4_runtimeText(str, x, y);
}

static void textUtf8 (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint8_t* str = getMemoryPointer(&params[0]);
    int32_t byteLength = params[1].i32_val;
    int32_t x = params[2].i32_val;
    int32_t y = params[3].i32_val;
    w4_runtimeTextUtf8(str, byteLength, x, y);
}

static void textUtf16 (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint16_t* str = getMemoryPointer(&params[0]);
    int32_t byteLength = params[1].i32_val;
    int32_t x = params[2].i32_val;
    int32_t y = params[3].i32_val;
    w4_runtimeTextUtf16(str, byteLength, x, y);
}

static void tone (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    int32_t frequency = params[0].i32_val;
    int32_t duration = params[1].i32_val;
    int32_t volume = params[2].i32_val;
    int32_t flags = params[3].i32_val;
    w4_runtimeTone(frequency, duration, volume, flags);
}

static void diskr (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    uint8_t* dest = getMemoryPointer(&params[0]);
    int32_t size = params[1].i32_val;
    returns[0].i32_val = w4_runtimeDiskr(dest, size);
}

static void diskw (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint8_t* src = getMemoryPointer(&params[0]);
    int32_t size = params[1].i32_val;
    returns[0].i32_val = w4_runtimeDiskw(src, size);
}

static void trace (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const char* str = getMemoryPointer(&params[0]);
    w4_runtimeTrace(str);
}

static void traceUtf8 (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint8_t* str = getMemoryPointer(&params[0]);
    int32_t byteLength = params[1].i32_val;
    w4_runtimeTraceUtf8(str, byteLength);
}

static void traceUtf16 (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const uint16_t* str = getMemoryPointer(&params[0]);
    int32_t byteLength = params[1].i32_val;
    w4_runtimeTraceUtf16(str, byteLength);
}

static void tracef (void* userdata, bb_module_instance* module, const bb_val* params, bb_val* returns)
{
    const char* str = getMemoryPointer(&params[0]);
    const void* stack = getMemoryPointer(&params[1]);
    w4_runtimeTracef(str, stack);
}

static void check (bb_error err) {
    if (err != BB_ERROR_OK) {
        fprintf(stderr, "WASM error: %s\n", bb_error_str(err));
        exit(1);
    }
}

enum
{
    W4_MEM_MAX_SIZE_BYTES = 64 * 1024,
};

static void* wasmMemoryResize(void* mem, size_t new_size_bytes, size_t old_size_bytes, void* userdata)
{
	if (new_size_bytes > W4_MEM_MAX_SIZE_BYTES)
	{
		return NULL;
	}
	return wasm_mem;
}

static void wasmMemoryFree(void* mem, size_t size_bytes, void* userdata)
{
	free(wasm_mem);
}

uint8_t* w4_wasmInit () {
	wasm_mem = malloc(W4_MEM_MAX_SIZE_BYTES);
    if (wasm_mem != 0) {
        memset(wasm_mem, 0, W4_MEM_MAX_SIZE_BYTES);
    }
	return wasm_mem;
}

void w4_wasmDestroy () {
	if (module_inst) {
		bb_module_instance_deinit(module_inst);
	}
	if (module_def) {
		bb_module_definition_deinit(module_def);
	}
	free(wasm_mem);
}

void w4_wasmLoadModule (const uint8_t* wasmBuffer, int byteLength) {
	bb_module_definition_init_opts def_opts = {
		.debug_name = "cart",
	};

	module_def = bb_module_definition_init(def_opts);
	check(bb_module_definition_decode(module_def, wasmBuffer, byteLength));

	wasm_package = bb_import_package_init("env");

	bb_valtype types[] = {
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
		BB_VALTYPE_I32,
	};

    bb_import_package_add_function(wasm_package, blit,			"blit",			types, 6, types, 0, NULL);
    bb_import_package_add_function(wasm_package, blitSub,		"blitSub",		types, 9, types, 0, NULL);
    bb_import_package_add_function(wasm_package, line,			"line",			types, 4, types, 0, NULL);
    bb_import_package_add_function(wasm_package, hline,			"hline",		types, 3, types, 0, NULL);
    bb_import_package_add_function(wasm_package, vline,			"vline",		types, 3, types, 0, NULL);
    bb_import_package_add_function(wasm_package, oval,			"oval",			types, 4, types, 0, NULL);
    bb_import_package_add_function(wasm_package, rect,			"rect",			types, 4, types, 0, NULL);
    bb_import_package_add_function(wasm_package, text,			"text",			types, 3, types, 0, NULL);
    bb_import_package_add_function(wasm_package, textUtf8,		"textUtf8",		types, 4, types, 0, NULL);
    bb_import_package_add_function(wasm_package, textUtf16,		"textUtf16",	types, 4, types, 0, NULL);

    bb_import_package_add_function(wasm_package, tone,			"tone",			types, 4, types, 0, NULL);

    bb_import_package_add_function(wasm_package, diskr,			"diskr",		types, 2, types, 1, NULL);
    bb_import_package_add_function(wasm_package, diskw,			"diskw",		types, 2, types, 1, NULL);

    bb_import_package_add_function(wasm_package, trace,			"trace",		types, 1, types, 0, NULL);
    bb_import_package_add_function(wasm_package, traceUtf8,		"traceUtf8",	types, 2, types, 0, NULL);
    bb_import_package_add_function(wasm_package, traceUtf16,	"traceUtf16",	types, 2, types, 0, NULL);
    bb_import_package_add_function(wasm_package, tracef,		"tracef",		types, 2, types, 0, NULL);

    bb_wasm_memory_config wasm_mem_config = {
    	.resize_callback = wasmMemoryResize,
    	.free_callback = wasmMemoryFree,
    };
	bb_import_package_add_memory(wasm_package, &wasm_mem_config, "memory", 1, 1);

	bb_module_instance_instantiate_opts inst_opts = {
		.packages = &wasm_package,
		.num_packages = 1,
		.stack_size = 64 * 1024,
	};

	module_inst = bb_module_instance_init(module_def);
	check(bb_module_instance_instantiate(module_inst, inst_opts));

	bb_func_handle func;
	if (bb_module_instance_find_func(module_inst, "_start", &func) == BB_ERROR_OK) {
		check(bb_module_instance_invoke(module_inst, func, NULL, 0, NULL, 0, (bb_module_instance_invoke_opts){0}));
	}
	if (bb_module_instance_find_func(module_inst, "_initialize", &func) == BB_ERROR_OK) {
		check(bb_module_instance_invoke(module_inst, func, NULL, 0, NULL, 0, (bb_module_instance_invoke_opts){0}));
	}

    bb_module_instance_find_func(module_inst, "start", &start);
    bb_module_instance_find_func(module_inst, "update", &update);
}

void w4_wasmCallStart () {
    if (bb_func_handle_isvalid(start)) {
        check(bb_module_instance_invoke(module_inst, start, NULL, 0, NULL, 0, (bb_module_instance_invoke_opts){0}));
    }
}

void w4_wasmCallUpdate () {
    if (bb_func_handle_isvalid(update)) {
        check(bb_module_instance_invoke(module_inst, update, NULL, 0, NULL, 0, (bb_module_instance_invoke_opts){0}));
    }
}
