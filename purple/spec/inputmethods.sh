#! /bin/sh
#
# Generate Docbook funcsynopsis-elements for a bunch of input-setters.
#

TYPES="boolean int32 uint32 real32 real32_vec2 real32_vec3 real32_vec4 real32_mat16 \
	real64 real64_vec2 real64_vec3 real64_vec4 real64_mat16 module string"

for t in $TYPES
do
	if [ "$t" = "module" ]; then
		TYPE="uint32"
	else
		TYPE="$t"
	fi
	cat <<-END
	<funcsynopsis>
	 <funcprototype>
	  <funcdef><function>mod_input_set_$t</function></funcdef>
	  <paramdef>uint32 <parameter>graph</parameter></paramdef>
	  <paramdef>uint32 <parameter>module</parameter></paramdef>
	  <paramdef>uint8 <parameter>input</parameter></paramdef>
	  <paramdef>$TYPE <parameter>value</parameter></paramdef>
	 </funcprototype>
	</funcsynopsis>
END
done
