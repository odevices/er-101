DEPFLAGS = -MF $@.d -MT $@ -MMD -MP

$(out_dir)/%.o: %.c
	@echo $(describe_env) C $(describe_input)
	@mkdir -p $(@D)
	@$(CC) $(DEPFLAGS) $(CFLAGS) -std=gnu99 -c $< -o $@

$(out_dir)/%.o: %.cpp
	@echo $(describe_env) C++ $(describe_input)
	@mkdir -p $(@D)
	@$(CPP) $(DEPFLAGS) $(CFLAGS) -std=gnu++99 -c $< -o $@

$(out_dir)/%.o: %.S
	@echo $(describe_env) ASM $(describe_input)
	@mkdir -p $(@D)
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

# Dependencies

#DEPFILES = $(objects:%.o=%o.d)
DEPFILES := $(call rwildcard,$(out_dir),*.d)
$(DEPFILES): ;
-include $(DEPFILES)