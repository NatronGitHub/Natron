
function printMessageWithDatePrefix() {
    echo "$(date -u '+%Y-%m-%d %H:%M:%S') $1"
}

function printStatusMessage() {
    printMessageWithDatePrefix "$1"
}

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
