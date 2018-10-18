/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <hypervisor.h>
#include <per_cpu.h>
/* buf size should be identical to the size in hvlog option, which is
 * transfered to SOS:
 * bsp/uefi/clearlinux/acrn.conf: hvlog=2M@0x1FE00000
 */
#define HVLOG_BUF_SIZE		(2*1024*1024)

struct logmsg {
	uint32_t flags;
	int32_t seq;
	spinlock_t lock;
};

static struct logmsg logmsg;

static inline void alloc_earlylog_sbuf(uint16_t pcpu_id)
{
	uint32_t ele_size = LOG_ENTRY_SIZE;
	uint32_t ele_num = (((HVLOG_BUF_SIZE >> 1U) / phys_cpu_num)
			   - SBUF_HEAD_SIZE) / ele_size;

	per_cpu(earlylog_sbuf, pcpu_id) = sbuf_allocate(ele_num, ele_size);
	if (per_cpu(earlylog_sbuf, pcpu_id) == NULL) {
		printf("failed to allcate sbuf for hvlog - %hu\n", pcpu_id);
	}
}

static inline void free_earlylog_sbuf(uint16_t pcpu_id)
{
	if (per_cpu(earlylog_sbuf, pcpu_id) == NULL) {
		return;
	}

	free(per_cpu(earlylog_sbuf, pcpu_id));
	per_cpu(earlylog_sbuf, pcpu_id) = NULL;
}

static void do_copy_earlylog(struct shared_buf *dst_sbuf,
			    struct shared_buf *src_sbuf)
{
	uint32_t buf_size, valid_size;
	uint32_t cur_tail;
	uint64_t rflags;

	if ((src_sbuf->ele_size != dst_sbuf->ele_size)
		&& (src_sbuf->ele_num != dst_sbuf->ele_num)) {
		spinlock_irqsave_obtain(&(logmsg.lock), &rflags);
		printf("Error to copy early hvlog: size mismatch\n");
		spinlock_irqrestore_release(&(logmsg.lock), rflags);
		return;
	}

	cur_tail = src_sbuf->tail;
	buf_size = SBUF_HEAD_SIZE + dst_sbuf->size;
	valid_size = SBUF_HEAD_SIZE + cur_tail;

	(void)memcpy_s((void *)dst_sbuf, buf_size,
			(void *)src_sbuf, valid_size);
	if (dst_sbuf->tail != cur_tail) {
		/* there is chance to lose new log from certain pcpu */
		dst_sbuf->tail = cur_tail;
	}
}

void init_logmsg(uint32_t flags)
{
	uint16_t pcpu_id;

	logmsg.flags = flags;
	logmsg.seq = 0;

	/* allocate sbuf for log before sos booting */
	for (pcpu_id = 0U; pcpu_id < phys_cpu_num; pcpu_id++) {
		alloc_earlylog_sbuf(pcpu_id);
	}
}

void do_logmsg(uint32_t severity, const char *fmt, ...)
{
	va_list args;
	uint64_t timestamp, rflags;
	uint16_t pcpu_id;
	bool do_console_log;
	bool do_mem_log;
	bool do_npk_log;
	char *buffer;

	do_console_log = (((logmsg.flags & LOG_FLAG_STDOUT) != 0U) &&
					(severity <= console_loglevel));
	do_mem_log = (((logmsg.flags & LOG_FLAG_MEMORY) != 0U) &&
					(severity <= mem_loglevel));
	do_npk_log = ((logmsg.flags & LOG_FLAG_NPK) != 0U &&
					(severity <= npk_loglevel));

	if (!do_console_log && !do_mem_log && !do_npk_log) {
		return;
	}

	/* Get time-stamp value */
	timestamp = rdtsc();

	/* Scale time-stamp appropriately */
	timestamp = ticks_to_us(timestamp);

	/* Get CPU ID */
	pcpu_id = get_cpu_id();
	buffer = per_cpu(logbuf, pcpu_id);

	(void)memset(buffer, 0U, LOG_MESSAGE_MAX_SIZE);
	/* Put time-stamp, CPU ID and severity into buffer */
	snprintf(buffer, LOG_MESSAGE_MAX_SIZE,
			"[%lluus][cpu=%hu][sev=%u][seq=%u]:",
			timestamp, pcpu_id, severity,
			atomic_inc_return(&logmsg.seq));

	/* Put message into remaining portion of local buffer */
	va_start(args, fmt);
	vsnprintf(buffer + strnlen_s(buffer, LOG_MESSAGE_MAX_SIZE),
		LOG_MESSAGE_MAX_SIZE
		- strnlen_s(buffer, LOG_MESSAGE_MAX_SIZE), fmt, args);
	va_end(args);

	/* Check if flags specify to output to NPK */
	if (do_npk_log) {
		npk_log_write(buffer, strnlen_s(buffer, LOG_MESSAGE_MAX_SIZE));
	}

	/* Check if flags specify to output to stdout */
	if (do_console_log) {
		spinlock_irqsave_obtain(&(logmsg.lock), &rflags);

		/* Send buffer to stdout */
		printf("%s\n\r", buffer);

		spinlock_irqrestore_release(&(logmsg.lock), rflags);
	}

	/* Check if flags specify to output to memory */
	if (do_mem_log) {
		unsigned int i, msg_len;
		struct shared_buf *sbuf = (struct shared_buf *)
					per_cpu(sbuf, pcpu_id)[ACRN_HVLOG];
		struct shared_buf *early_sbuf = per_cpu(earlylog_sbuf, pcpu_id);

		if (early_sbuf != NULL) {
			if (sbuf != NULL) {
				/* switch to sbuf from sos */
				do_copy_earlylog(sbuf, early_sbuf);
				free_earlylog_sbuf(pcpu_id);
			} else {
				/* use earlylog sbuf if no sbuf from sos */
				sbuf = early_sbuf;
			}
		}

		if (sbuf != NULL) {
			msg_len = strnlen_s(buffer, LOG_MESSAGE_MAX_SIZE);

			for (i = 0U; i < (((msg_len - 1U) / LOG_ENTRY_SIZE) + 1U);
					i++) {
				(void)sbuf_put(sbuf, (uint8_t *)buffer +
							(i * LOG_ENTRY_SIZE));
			}
		}
	}
}

void print_logmsg_buffer(uint16_t pcpu_id)
{
	char buffer[LOG_ENTRY_SIZE + 1];
	uint32_t read_cnt;
	struct shared_buf **sbuf;
	int is_earlylog = 0;
	uint64_t rflags;

	if (pcpu_id >= phys_cpu_num) {
		return;
	}

	if (per_cpu(earlylog_sbuf, pcpu_id) != NULL) {
		sbuf = &per_cpu(earlylog_sbuf, pcpu_id);
		is_earlylog = 1;
	} else {
		sbuf = (struct shared_buf **)
				&per_cpu(sbuf, pcpu_id)[ACRN_HVLOG];
	}

	spinlock_irqsave_obtain(&(logmsg.lock), &rflags);
	if ((*sbuf) != NULL) {
		printf("CPU%hu: head: 0x%x, tail: 0x%x %s\n\r",
			pcpu_id, (*sbuf)->head, (*sbuf)->tail,
			(is_earlylog != 0) ? "[earlylog]" : "");
	}
	spinlock_irqrestore_release(&(logmsg.lock), rflags);

	do {
		uint32_t idx;
		(void)memset(buffer, 0U, LOG_ENTRY_SIZE + 1U);

		if ((*sbuf == NULL) || (buffer == NULL)) {
			return;
		}

		read_cnt = sbuf_get(*sbuf, (uint8_t *)buffer);

		if (read_cnt == 0U) {
			return;
		}

		idx = ((uint32_t)read_cnt < LOG_ENTRY_SIZE) ?
				(uint32_t)read_cnt : LOG_ENTRY_SIZE;
		buffer[idx] = '\0';

		spinlock_irqsave_obtain(&(logmsg.lock), &rflags);
		printf("%s\n\r", buffer);
		spinlock_irqrestore_release(&(logmsg.lock), rflags);
	} while (read_cnt > 0U);
}