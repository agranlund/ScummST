cd backends/atari/libcmini
make
cd ../../..
make game=monkey1 clean
make game=monkey1
make game=monkey2 clean
make game=monkey2
make game=atlantis clean
make game=atlantis
make game=tentacle clean
make game=tentacle
make game=samnmax clean
make game=samnmax
cp -rf ./LICENSE ./bin/LICENSE.TXT
cp -rf ./README.md ./bin/README.TXT
cp -rf ./ISSUES.TXT ./bin/ISSUES.TXT
