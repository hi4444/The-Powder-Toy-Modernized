#include "FontReader.h"

#include "bzip2/bz2wrap.h"
#include "font_bz2.h"

#include <array>
#include <cstdint>
#include <vector>

unsigned char *font_data = nullptr;
unsigned int *font_ptrs = nullptr;
unsigned int (*font_ranges)[2] = nullptr;
unsigned int *font_range_offsets = nullptr;
int font_ranges_count = 0;

FontReader::FontReader(unsigned char const *_pointer):
	pointer(_pointer + 1),
	width(*_pointer),
	pixels(0),
	data(0)
{
}

static bool InitFontData()
{
	static std::vector<char> fontDataBuf;
	static std::vector<int> fontPtrsBuf;
	static std::vector< std::array<int, 2> > fontRangesBuf;
	static std::vector<unsigned int> fontRangeOffsetsBuf;
	if (BZ2WDecompress(fontDataBuf, font_bz2.AsCharSpan()) != BZ2WDecompressOk)
	{
		return false;
	}
	int first = -1;
	int last = -1;
	unsigned int offset = 0;
	char *begin = fontDataBuf.data();
	char *ptr = fontDataBuf.data();
	char *end = fontDataBuf.data() + fontDataBuf.size();
	while (ptr != end)
	{
		if (ptr + 4 > end)
		{
			return false;
		}
		auto codePoint = *reinterpret_cast<uint32_t *>(ptr) & 0xFFFFFFU;
		if (codePoint >= 0x110000U)
		{
			return false;
		}
		auto width = *reinterpret_cast<uint8_t *>(ptr + 3);
		if (width > 64)
		{
			return false;
		}
		if (ptr + 4 + width * 3 > end)
		{
			return false;
		}
		auto cp = (int)codePoint;
		if (last >= cp)
		{
			return false;
		}
		if (first != -1 && last + 1 < cp)
		{
			fontRangesBuf.push_back({ { first, last } });
			fontRangeOffsetsBuf.push_back(offset);
			offset += unsigned(last - first + 1);
			first = -1;
		}
		if (first == -1)
		{
			first = cp;
		}
		last = cp;
		fontPtrsBuf.push_back(ptr + 3 - begin);
		ptr += width * 3 + 4;
	}
	if (first != -1)
	{
		fontRangesBuf.push_back({ { first, last } });
		fontRangeOffsetsBuf.push_back(offset);
	}
	fontRangesBuf.push_back({ { 0, 0 } });
	font_data = reinterpret_cast<unsigned char *>(fontDataBuf.data());
	font_ptrs = reinterpret_cast<unsigned int *>(fontPtrsBuf.data());
	font_ranges = reinterpret_cast<unsigned int (*)[2]>(fontRangesBuf.data());
	font_range_offsets = fontRangeOffsetsBuf.data();
	font_ranges_count = int(fontRangeOffsetsBuf.size());
	return true;
}

unsigned char const *FontReader::lookupChar(String::value_type ch)
{
	if (!font_data)
	{
		if (!InitFontData())
		{
			throw std::runtime_error("font data corrupt");
		}
	}
	int left = 0;
	int right = font_ranges_count - 1;
	while (left <= right)
	{
		int mid = (left + right) / 2;
		auto rangeStart = font_ranges[mid][0];
		auto rangeEnd = font_ranges[mid][1];
		if (ch < rangeStart)
		{
			right = mid - 1;
		}
		else if (ch > rangeEnd)
		{
			left = mid + 1;
		}
		else
		{
			auto index = font_range_offsets[mid] + (ch - rangeStart);
			return &font_data[font_ptrs[index]];
		}
	}
	if (ch == 0xFFFD)
		return &font_data[0];
	return lookupChar(0xFFFD);
}

FontReader::FontReader(String::value_type ch):
	FontReader(lookupChar(ch))
{
}

int FontReader::GetWidth() const
{
	return width;
}

int FontReader::NextPixel()
{
	if(!pixels)
	{
		data = *(pointer++);
		pixels = 4;
	}
	int old = data;
	pixels--;
	data >>= 2;
	return old & 0x3;
}
