/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h> // Necesario para memset

#include "mgos_neopixel.h"
#include "mgos_gpio.h"
#include "mgos_system.h"
#include "mgos_bitbang.h"
#include "common/cs_dbg.h"

#define NUM_CHANNELS 3 /* r, g, b */

struct mgos_neopixel
{
  int pin;
  int num_pixels;
  enum mgos_neopixel_order order;
  uint8_t *data;
};

struct mgos_neopixel *mgos_neopixel_create(int pin, int num_pixels,
                                           enum mgos_neopixel_order order)
{
  mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_OUTPUT);
  /* Mantener el pin en reset */
  mgos_gpio_write(pin, 0);

  struct mgos_neopixel *np = calloc(1, sizeof(*np));
  if (np == NULL)
  {
    LOG(LL_ERROR, ("Error al asignar memoria para mgos_neopixel"));
    return NULL;
  }
  np->pin = pin;
  np->num_pixels = num_pixels;
  np->order = order;
  np->data = malloc(num_pixels * NUM_CHANNELS);
  if (np->data == NULL)
  {
    LOG(LL_ERROR, ("Error al asignar memoria para los datos de neopixel"));
    free(np);
    return NULL;
  }
  mgos_neopixel_clear(np);
  return np;
}

void mgos_neopixel_set(struct mgos_neopixel *np, int i, int r, int g, int b)
{
  if (i < 0 || i >= np->num_pixels)
  {
    LOG(LL_ERROR, ("Índice %d fuera de rango (0 - %d)", i, np->num_pixels - 1));
    return;
  }
  uint8_t *p = np->data + i * NUM_CHANNELS;
  switch (np->order)
  {
  case MGOS_NEOPIXEL_ORDER_RGB:
    p[0] = r;
    p[1] = g;
    p[2] = b;
    break;
  case MGOS_NEOPIXEL_ORDER_GRB:
    p[0] = g;
    p[1] = r;
    p[2] = b;
    break;
  case MGOS_NEOPIXEL_ORDER_BGR:
    p[0] = b;
    p[1] = g;
    p[2] = r;
    break;
  default:
    LOG(LL_ERROR, ("Orden de píxeles incorrecto: %d", np->order));
    break;
  }
}

void mgos_neopixel_clear(struct mgos_neopixel *np)
{
  memset(np->data, 0, np->num_pixels * NUM_CHANNELS);
}

/*
 * La función mgos_neopixel_show envía los datos a la tira de LEDs. Si la funcionalidad
 * bitbang está habilitada (MGOS_ENABLE_BITBANG), se comprueba que esté lista antes de
 * enviar los datos mediante mgos_bitbang_write_bits.
 */
void mgos_neopixel_show(struct mgos_neopixel *np)
{
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(300);
#if MGOS_ENABLE_BITBANG
  if (mgos_bitbang_is_ready())
  { // Se asume que existe esta función de comprobación
    mgos_bitbang_write_bits(np->pin, MGOS_DELAY_100NSEC, 3, 8, 8, 3,
                            np->data, np->num_pixels * NUM_CHANNELS);
  }
  else
  {
    LOG(LL_ERROR, ("Bitbang no está listo"));
  }
#endif
  mgos_gpio_write(np->pin, 0);
  mgos_usleep(300);
  mgos_gpio_write(np->pin, 1);

   /*uint8_t data = 0;
   mgos_bitbang_write_bits(np->pin, MGOS_DELAY_USEC, -1, 10, -1, 10, &data, 1);
   mgos_bitbang_write_bits(np->pin, MGOS_DELAY_100NSEC, 3, 8, 8, 3, np->data, np->num_pixels * NUM_CHANNELS);
   mgos_bitbang_write_bits(np->pin, MGOS_DELAY_USEC, -1, 10, -1, 10, &data, 1);
   mgos_bitbang_write_bits(np->pin, MGOS_DELAY_USEC, 10, -1, 10, -1, &data, 1);*/
}

void mgos_neopixel_free(struct mgos_neopixel *np)
{
  if (np != NULL)
  {
    free(np->data);
    free(np);
  }
}

bool mgos_neopixel_init(void)
{
  return true;
}

void mgos_neopixel_fill(struct mgos_neopixel *np, int i, int j, int r, int g, int b)
{
  int end = i + j;
  if (i >= np->num_pixels)
  {
    return; // Si el primer LED está fuera del rango, no se realiza ninguna acción
  }
  if (j == 0 || end > np->num_pixels)
  {
    end = np->num_pixels;
  }
  for (int index = i; index < end; index++)
  {
    mgos_neopixel_set(np, index, r, g, b);
  }
}
