
function printMessageWithDatePrefix() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $1"
}

function printStatusMessage() {
    printMessageWithDatePrefix "$1"
}
