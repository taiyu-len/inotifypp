CXXFLAGS := -std=c++17 -Wall -Wextra -march=native
LDLIBS=-lstdc++fs -lpthread -lboost_system
CPPFLAGS := -Iinclude
DEBUG ?= 1
ifeq ($(DEBUG), 1)
  CXXFLAGS += -Og -ggdb -fsanitize=address,undefined
else
  CPPFLAGS += -DNDEBUG -DDOCTEST_CONFIG_DISABLE
endif

SOURCES := event.cpp event_buffer.cpp instance.cpp
SOURCES += main.cpp test.cpp test_async.cpp
SOURCES := $(addprefix src/inotifypp/,$(SOURCES))
OBJECTS := $(addsuffix .o,$(basename $(SOURCES)))
DEPENDS := $(addsuffix .d,$(basename $(SOURCES)))

tests: $(OBJECTS)
	$(LINK.cc) $^ $(LDLIBS) -o $@

test: tests
	./tests

ifeq ($(MAKECMDGOALS),clean)
else ifeq ($(MAKECMDGOALS),mostlyclean)
else
-include $(DEPENDS)
%.d: %.cpp; @$(CXX) $(CPPFLAGS) $< -MM -MT $*.o -MT $@ > $@
endif

mostlyclean:
	@rm -f $(OBJECTS) $(DEPENDS)
clean: mostlyclean
	@rm -f tests
