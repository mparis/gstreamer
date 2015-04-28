/*
 * (C) Copyright 2015 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "kmslatency.h"

GST_DEBUG_CATEGORY_STATIC (kms_latency_debug);
#define GST_CAT_DEFAULT kms_latency_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (kms_latency_debug, "kmslatency", 0, "latency tracer");
#define kms_latency_tracer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (KmsLatencyTracer, kms_latency_tracer, GST_TYPE_TRACER,
    _do_init);

#define KMS_LATENCY_TRACER_GET_PRIVATE(obj) (  \
  G_TYPE_INSTANCE_GET_PRIVATE (                \
    (obj),                                     \
    KMS_TYPE_LATENCY_TRACER,                   \
    KmsLatencyTracerPrivate                    \
  )                                            \
)

typedef enum KmsLatencyEntryType
{
  BUFFER_PRE,
  BUFFER_POST,
  BUFFER_LIST_PRE,
  BUFFER_LIST_POST,
} KmsLatencyEntryType;

/* *INDENT-OFF* */
const static struct {
  KmsLatencyEntryType type;
  const char *str;
} entry_type_str [] = {
  {BUFFER_PRE, "BUFFER_PRE"},
  {BUFFER_POST, "BUFFER_POST"},
  {BUFFER_LIST_PRE, "BUFFER_LIST_PRE"},
  {BUFFER_LIST_POST, "BUFFER_LIST_POST"},
};
/* *INDENT-ON* */

typedef struct KmsLatencyEntry
{
  gpointer thread_ptr;
  KmsLatencyEntryType type;
  GstElement *element;
  GstPad *pad;
  guint64 ts;
  guint num_buffers;
} KmsLatencyEntry;

struct _KmsLatencyTracerPrivate
{
  gint idx;
  KmsLatencyEntry *entries;
  gboolean entry_limit_reached;
};

#define MAX_ENTRIES_NUM 10000000        /* TODO: do configurable */
#define MAX_ENTRIES_SIZE MAX_ENTRIES_NUM * sizeof (KmsLatencyEntry)

static void
do_push_buffer_pre (GstTracer * tracer, guint64 ts, GstPad * pad,
    GstBuffer * buf)
{
  KmsLatencyTracer *self = KMS_LATENCY_TRACER (tracer);
  KmsLatencyEntry *entry;
  gint i;

  if (self->priv->idx >= MAX_ENTRIES_NUM) {
    self->priv->entry_limit_reached = TRUE;
    return;
  }

  i = g_atomic_int_add (&self->priv->idx, 1);

  entry = &self->priv->entries[i];
  entry->thread_ptr = g_thread_self ();
  entry->type = BUFFER_PRE;
  entry->element = gst_pad_get_parent_element (pad);
  entry->pad = g_object_ref (pad);
  entry->ts = ts;
  entry->num_buffers = 1;
}

static void
do_push_buffer_post (GstTracer * tracer, guint64 ts, GstPad * pad,
    GstFlowReturn res)
{
  KmsLatencyTracer *self = KMS_LATENCY_TRACER (tracer);
  KmsLatencyEntry *entry;
  gint i;

  if (self->priv->idx >= MAX_ENTRIES_NUM) {
    self->priv->entry_limit_reached = TRUE;
    return;
  }

  i = g_atomic_int_add (&self->priv->idx, 1);

  entry = &self->priv->entries[i];
  entry->thread_ptr = g_thread_self ();
  entry->type = BUFFER_POST;
  entry->element = gst_pad_get_parent_element (pad);
  entry->pad = g_object_ref (pad);
  entry->ts = ts;
}

static void
do_push_buffer_list_pre (GstTracer * tracer, guint64 ts, GstPad * pad,
    GstBufferList * buf_list)
{
  KmsLatencyTracer *self = KMS_LATENCY_TRACER (tracer);
  KmsLatencyEntry *entry;
  gint i;

  if (self->priv->idx >= MAX_ENTRIES_NUM) {
    self->priv->entry_limit_reached = TRUE;
    return;
  }

  i = g_atomic_int_add (&self->priv->idx, 1);

  entry = &self->priv->entries[i];
  entry->thread_ptr = g_thread_self ();
  entry->type = BUFFER_LIST_PRE;
  entry->element = gst_pad_get_parent_element (pad);
  entry->pad = g_object_ref (pad);
  entry->ts = ts;
  entry->num_buffers = gst_buffer_list_length (buf_list);
}

static void
do_push_buffer_list_post (GstTracer * tracer, guint64 ts, GstPad * pad,
    GstFlowReturn res)
{
  KmsLatencyTracer *self = KMS_LATENCY_TRACER (tracer);
  KmsLatencyEntry *entry;
  gint i;

  if (self->priv->idx >= MAX_ENTRIES_NUM) {
    self->priv->entry_limit_reached = TRUE;
    return;
  }

  i = g_atomic_int_add (&self->priv->idx, 1);

  entry = &self->priv->entries[i];
  entry->thread_ptr = g_thread_self ();
  entry->type = BUFFER_LIST_POST;
  entry->element = gst_pad_get_parent_element (pad);
  entry->pad = g_object_ref (pad);
  entry->ts = ts;
}

static void
kms_latency_tracer_log_entry (KmsLatencyTracer * self, KmsLatencyEntry * entry)
{
  const gchar *element_name =
      (entry->element != NULL) ? GST_ELEMENT_NAME (entry->element) : "";

  GST_TRACE_OBJECT (self,
      "%p,%s,%s:%s,%" G_GUINT64_FORMAT ",%" G_GUINT32_FORMAT, entry->thread_ptr,
      entry_type_str[entry->type].str, element_name, GST_PAD_NAME (entry->pad),
      entry->ts, entry->num_buffers);
}

static void
kms_latency_tracer_finalize (GObject * object)
{
  KmsLatencyTracer *self = KMS_LATENCY_TRACER (object);
  gint i;

  GST_DEBUG_OBJECT (self, "finalize");

  if (self->priv->entry_limit_reached) {
    GST_WARNING_OBJECT (self, "Entry number (%d) limit was reached",
        MAX_ENTRIES_NUM);
  }

  GST_INFO_OBJECT (self, "Logging out profiling data...");
  for (i = 0; i < self->priv->idx; i++) {
    KmsLatencyEntry entry = self->priv->entries[i];
    kms_latency_tracer_log_entry (self, &entry);
    g_clear_object (&entry.element);
    g_clear_object (&entry.pad);
  }
  GST_INFO_OBJECT (self, "Logging out OK.");

  g_slice_free1 (MAX_ENTRIES_SIZE, self->priv->entries);

  /* chain up */
  G_OBJECT_CLASS (kms_latency_tracer_parent_class)->finalize (object);
}

static void
kms_latency_tracer_class_init (KmsLatencyTracerClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = kms_latency_tracer_finalize;

  /* announce trace formats */
  /* *INDENT-OFF* */
  gst_tracer_log_trace (gst_structure_new ("latency.class",
      "src", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "related-to", G_TYPE_STRING, "pad",  /* TODO: use genum */
          NULL),
      "sink", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "related-to", G_TYPE_STRING, "pad",  /* TODO: use genum */
          NULL),
      "time", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_UINT64,
          "description", G_TYPE_STRING,
              "time it took for the buffer to go from src to sink ns",
          "flags", G_TYPE_STRING, "aggregated",  /* TODO: use gflags */ 
          "min", G_TYPE_UINT64, G_GUINT64_CONSTANT (0),
          "max", G_TYPE_UINT64, G_MAXUINT64,
          NULL),
      NULL));
  /* *INDENT-ON* */

  g_type_class_add_private (klass, sizeof (KmsLatencyTracerPrivate));
}

static void
kms_latency_tracer_init (KmsLatencyTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);

  GST_INFO_OBJECT (self, "Init...");

  self->priv = KMS_LATENCY_TRACER_GET_PRIVATE (self);
  self->priv->entries = g_slice_alloc0 (MAX_ENTRIES_SIZE);

  gst_tracing_register_hook (tracer, "pad-push-pre",
      G_CALLBACK (do_push_buffer_pre));
  gst_tracing_register_hook (tracer, "pad-push-post",
      G_CALLBACK (do_push_buffer_post));
  gst_tracing_register_hook (tracer, "pad-push-list-pre",
      G_CALLBACK (do_push_buffer_list_pre));
  gst_tracing_register_hook (tracer, "pad-push-list-post",
      G_CALLBACK (do_push_buffer_list_post));

  GST_INFO_OBJECT (self, "Init OK.");
}
