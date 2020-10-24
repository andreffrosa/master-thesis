PROJ_DIR=".."
find $PROJ_DIR/CommunicationPrimitives/ -regex '.*\(c\|h\)$' | xargs wc -l | sort -nr | grep total
