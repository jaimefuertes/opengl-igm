todo: spinningcube_withlight

LDLIBS=-lGL -lGLEW -lglfw -lm 

spinningcube_withlight: spinningcube_withlight.cpp textfile.c
	gcc $^ -lGL -lGLEW -lglfw -lm -o $@

clean:
	rm -f *.o *~

cleanall: clean
	rm -f spinningcube_withlight
