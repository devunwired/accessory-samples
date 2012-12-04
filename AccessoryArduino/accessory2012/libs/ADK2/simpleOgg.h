/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef ADK_INTERNAL
#ifndef _SIMPLE_OGG_H_
#define _SIMPLE_OGG_H_


#define SIMPLE_OGG_OK			0
#define SIMPLE_OGG_OPEN_ERR		1
#define SIMPLE_OFF_OGG_OPEN_ERR		2
#define SIMPLE_OGG_FILE_ERR		3
#define SIMPLE_OGG_MEM_ERR		4

char playOgg(const char* name);
char playOggBackground(const char* name, char *complete, char *abort);

#endif
#endif

