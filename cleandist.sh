rm backends/atari/tools/gst2ascii
rm backends/atari/tools/stripx

cd backends/atari/libcmini
make clean
rm -rf build
cd ../../..
rm -r backends/sdl/*.o
rm profile.tmp
rm *.stackdump

make game=monkey1 clean
make game=monkey2 clean
make game=atlantis clean
make game=tentacle clean
make game=samnmax clean

