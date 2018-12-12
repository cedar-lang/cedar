#include <cedar.hh>


using namespace cedar;



#define ALIGNMENT 8
// rounds up to the nearest multiple of ALIGNMENT
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


void runes::init(uint32_t size) {
	cap = ALIGN(size);
	len = 0;
	buf = new rune[cap];

	for (int i = 0; i < cap; i++) buf[i] = 0;
}

void runes::resize(uint32_t size, rune c) {
	uint32_t new_cap = ALIGN(size);

	if (new_cap > cap) {
		new_cap = ALIGN(cap * 2);

		rune *new_buf = new rune[new_cap];
		for (int i = 0; i < cap; i++)
			new_buf[i] = buf[i];

		for (int i = cap; i < new_cap; i++)
			new_buf[i] = c;

		delete buf;
		buf = new_buf;
		cap = new_cap;
	}
}


void runes::ingest_utf8(std::string s) {
	init(1);
	len = 0;
	std::string corrected;
	utf8::replace_invalid(s.begin(), s.end(), back_inserter(corrected));
	utf8::utf8to32(corrected.begin(), corrected.end(), std::back_inserter(*this));
}


runes::runes() {
	init();
}


runes::runes(const char *s) {
	ingest_utf8(std::string(s));
}


runes::runes(const char32_t *s) {
	uint32_t l;
	for (l = 0; s[l]; l++);
	init(l);
	len = l;
	for (int i = 0; i < l; i++) buf[i] = s[i];
}

runes::runes(std::string s) {
	ingest_utf8(s);
}


runes::~runes() {
	// delete buf;
}

rune *runes::begin(void) { return buf; }

rune *runes::end(void) {
	return buf + len + 1;
}

const rune *runes::cbegin(void) { return buf; }

const rune *runes::cend(void) { return buf + len + 1; }

uint32_t runes::size(void) { return len; }

uint32_t runes::length(void) { return len; }

uint32_t runes::max_size(void) { return -1; }

uint32_t runes::capacity(void) { return cap; }

void runes::clear(void) {
	for (int i = 0; i < len; i++) buf[i] = '\0';
	len = 0;
}

bool runes::empty(void) {
	return len == 0;
}


runes& runes::operator+=(const runes& other) {
	if (other.len + len >= cap) {
		resize(other.len + len);
	}

	for (int i = 0; i < other.len; i++) {
		buf[len + i] = other.buf[i];
	}

	len += other.len;

	return *this;
}

runes& runes::operator+=(const char *other) {
	return operator+=(runes(other));
}


runes& runes::operator+=(std::string other) {
	return operator+=(runes(other));
}


runes::operator std::string() {
	std::string s;
	utf8::utf32to8(begin(), end(), std::back_inserter(s));
	return s;
}


rune runes::operator[](size_t pos) {
	return *(buf + pos);
}

void runes::push_back(rune& r) {
	if (len >= cap) {
		resize(len+1);
	}
	buf[len] = r;
	len += 1;
}

void runes::push_back(rune&& r) {
	if (len >= cap) {
		resize(len+1);
	}
	buf[len] = r;
	len += 1;
}
