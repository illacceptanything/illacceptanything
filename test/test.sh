for pc in $(seq 1 9000); do
    printf "F"
    sleep 1
done
echo
echo
echo "All tests failed in 9000.0 seconds."
echo "Done."
