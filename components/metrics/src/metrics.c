#include "libmcu/metrics.h"
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include "libmcu/compiler.h"

#define ARRAY_LEN(arr)		(sizeof(arr) / sizeof(arr[0]))
#define METRICS_LEN		ARRAY_LEN(metrics)

struct metrics_s {
	const uint8_t key_id;
	int32_t value;
};

static pthread_mutex_t metrics_lock;
static struct metrics_s metrics[] = {
#define METRICS_DEFINE(id, key) (struct metrics_s){ .key_id = id, .value = 0, },
#include METRICS_USER_DEFINES
#undef METRICS_DEFINE
};

static struct metrics_s *get_obj_from_index(uint32_t index)
{
	return &metrics[index];
}

static uint32_t get_index_from_key(metric_key_t key)
{
	for (uint32_t i = 0; i < METRICS_LEN; i++) {
		if (get_obj_from_index(i)->key_id == key) {
			return i;
		}
	}

	assert(0);
}

static struct metrics_s *get_obj_from_key(metric_key_t key)
{
	return get_obj_from_index(get_index_from_key(key));
}

static void iterate_all(void (*callback_each)(metric_key_t key, int32_t value))
{
	for (uint32_t i = 0; i < METRICS_LEN; i++) {
		const struct metrics_s *p = get_obj_from_index(i);
		callback_each(p->key_id, p->value);
	}
}

static void reset_all(void)
{
	for (uint32_t i = 0; i < METRICS_LEN; i++) {
		get_obj_from_index(i)->value = 0;
	}
}

// TODO: get the default encoder for CBOR
LIBMCU_WEAK size_t metrics_encode(void *buf, size_t bufsize,
		metric_key_t key, int32_t value)
{
	unused(key);

	if (bufsize < sizeof(value)) {
		return 0;
	}

	memcpy(buf, &value, sizeof(value));

	return sizeof(value);
}

static size_t encode_all(uint8_t *buf, size_t bufsize)
{
	size_t written = 0;

	for (uint32_t i = 0; i < METRICS_LEN; i++) {
		const struct metrics_s *p = get_obj_from_index(i);
		written += metrics_encode(&buf[written], bufsize - written,
				p->key_id, p->value);
	}

	return written;
}

void metrics_set(metric_key_t key, int32_t val)
{
	pthread_mutex_lock(&metrics_lock);
	{
		get_obj_from_key(key)->value = val;
	}
	pthread_mutex_unlock(&metrics_lock);
}

void metrics_increase(metric_key_t key)
{
	pthread_mutex_lock(&metrics_lock);
	{
		get_obj_from_key(key)->value++;
	}
	pthread_mutex_unlock(&metrics_lock);
}

void metrics_increase_by(metric_key_t key, int32_t n)
{
	pthread_mutex_lock(&metrics_lock);
	{
		get_obj_from_key(key)->value += n;
	}
	pthread_mutex_unlock(&metrics_lock);
}

void metrics_reset(void)
{
	pthread_mutex_lock(&metrics_lock);
	{
		reset_all();
	}
	pthread_mutex_unlock(&metrics_lock);
}

size_t metrics_get_encoded(void *buf, size_t bufsize)
{
	size_t written;

	pthread_mutex_lock(&metrics_lock);
	{
		written = encode_all(buf, bufsize);
	}
	pthread_mutex_unlock(&metrics_lock);

	return written;
}

void metrics_iterate(void (*callback_each)(metric_key_t key, int32_t value))
{
	iterate_all(callback_each);
}

void metrics_init(void)
{
	reset_all();
	int rc = pthread_mutex_init(&metrics_lock, NULL);
	assert(rc == 0);
}
