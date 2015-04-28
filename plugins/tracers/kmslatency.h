/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
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

#ifndef __KMS_LATENCY_TRACER_H__
#define __KMS_LATENCY_TRACER_H__

#include <gst/gst.h>
#include <gst/gsttracer.h>

G_BEGIN_DECLS

#define KMS_TYPE_LATENCY_TRACER \
  (kms_latency_tracer_get_type())
#define KMS_LATENCY_TRACER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),KMS_TYPE_LATENCY_TRACER,KmsLatencyTracer))
#define KMS_LATENCY_TRACER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),KMS_TYPE_LATENCY_TRACER,KmsLatencyTracerClass))
#define KMS_IS_LATENCY_TRACER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),KMS_TYPE_LATENCY_TRACER))
#define KMS_IS_LATENCY_TRACER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),KMS_TYPE_LATENCY_TRACER))
#define KMS_LATENCY_TRACER_CAST(obj) ((KmsLatencyTracer *)(obj))

typedef struct _KmsLatencyTracerPrivate KmsLatencyTracerPrivate;
typedef struct _KmsLatencyTracer KmsLatencyTracer;
typedef struct _KmsLatencyTracerClass KmsLatencyTracerClass;

struct _KmsLatencyTracer {
  GstTracer 	 parent;

  KmsLatencyTracerPrivate *priv;
};

struct _KmsLatencyTracerClass {
  GstTracerClass parent_class;
};

G_GNUC_INTERNAL GType kms_latency_tracer_get_type (void);

G_END_DECLS

#endif /* __KMS_LATENCY_TRACER_H__ */
