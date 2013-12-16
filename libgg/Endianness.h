#pragma once

enum Endianness
{
	Endianness_LE,
	Endianness_BE,
	Endianness_iPhone = Endianness_LE,
	Endianness_x86 = Endianness_BE,
	Endianness_x64 = Endianness_BE,
	Endianness_Host = Endianness_BE // todo: auto determine type
};

Endianness HostEndianness_get();

void HostToSpec(void* src, void* dst, Endianness e);
void SpecToHost(void* src, void* dst, Endianness e);