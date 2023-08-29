#pragma once

#include "profile_data.h"

namespace Profile::Instant
{
	void init();
	bool CheckValid();
	bool refreshData();

	uint32_t availableDataSize();
	uint32_t getSize();
	uint32_t getHash();
	const Data* getData();
}

