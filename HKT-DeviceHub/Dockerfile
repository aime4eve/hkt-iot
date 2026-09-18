FROM eclipse-temurin:17-jre
WORKDIR /app

COPY target/devicehub-*.jar ./

RUN latest=$(ls devicehub-*.jar | sort -V | tail -1) && \
    mv "$latest" app.jar && \
    rm -f devicehub-*.jar

EXPOSE 8080
CMD ["java", "-jar", "app.jar"]
