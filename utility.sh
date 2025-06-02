argc=$#

show_usage_and_exit() {
    echo "usage: ./utility.sh [help | build | test] <args...>"
    echo "Sample 1: ./utility.sh build <CMakePresets.json preset name>"
    echo "Sample 2: ./utility.sh test"
    exit $1
}

if [[ $argc -lt 1 ]]; then
    show_usage_and_exit 1
fi

action="$1"

if [[ $action = "help" ]]; then
    show_usage_and_exit 0
elif [[ $action = "build" ]]; then
    cmake --fresh -S . -B build --preset "$2" && cmake --build build && mv ./build/compile_commands.json .
elif [[ $action = "test" ]]; then
    ctest --test-dir build --timeout 3 -V
else 
    show_usage_and_exit 1
fi
